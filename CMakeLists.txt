cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

option(BUILD_TESTING "Build tests" OFF)
option(USE_PKGCONFIG "Use pkg-config to find dependencies" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

if(VCPKG_TARGET_TRIPLET)
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

  find_package(Backward CONFIG)

  find_package(CURL CONFIG REQUIRED)
  find_package(fmt CONFIG REQUIRED)
  find_package(ncnn CONFIG REQUIRED)

  add_library(ncnn::ncnn ALIAS ncnn)
elseif(USE_PKGCONFIG)
  find_package(PkgConfig REQUIRED)

  # --- CURL ---
  pkg_check_modules(PC_CURL REQUIRED libcurl)
  add_library(CURL::libcurl INTERFACE IMPORTED)
  target_link_libraries(CURL::libcurl INTERFACE ${PC_CURL_LIBRARIES})
  target_include_directories(CURL::libcurl INTERFACE ${PC_CURL_INCLUDE_DIRS})
  target_compile_definitions(CURL::libcurl INTERFACE ${PC_CURL_CFLAGS_OTHER})
  target_link_directories(CURL::libcurl INTERFACE ${PC_CURL_LIBRARY_DIRS})

  # --- fmt ---
  pkg_check_modules(PC_FMT REQUIRED fmt)
  add_library(fmt::fmt INTERFACE IMPORTED)
  target_link_libraries(fmt::fmt INTERFACE ${PC_FMT_LIBRARIES})
  target_include_directories(fmt::fmt INTERFACE ${PC_FMT_INCLUDE_DIRS})
  target_compile_definitions(fmt::fmt INTERFACE ${PC_FMT_CFLAGS_OTHER})
  target_link_directories(fmt::fmt INTERFACE ${PC_FMT_LIBRARY_DIRS})

  # --- ncnn ---
  pkg_check_modules(PC_NCNN REQUIRED ncnn)
  add_library(ncnn::ncnn INTERFACE IMPORTED)
  target_link_libraries(ncnn::ncnn INTERFACE ${PC_NCNN_LIBRARIES})
  target_include_directories(ncnn::ncnn INTERFACE ${PC_NCNN_INCLUDE_DIRS})
  target_compile_definitions(ncnn::ncnn INTERFACE ${PC_NCNN_CFLAGS_OTHER} PREFIXED_NCNN_HEADERS)
  target_link_directories(ncnn::ncnn INTERFACE ${PC_NCNN_LIBRARY_DIRS})
else()
  message(FATAL_ERROR "Either USE_PKGCONFIG or VCPKG_TARGET_TRIPLET must be set.")
endif()

add_library(Logger INTERFACE)
target_include_directories(Logger INTERFACE ${CMAKE_SOURCE_DIR}/src/Logger)
target_link_libraries(Logger INTERFACE fmt::fmt)
target_sources(Logger PRIVATE src/Logger/ILogger.hpp)

add_library(Memory INTERFACE)
target_include_directories(Memory INTERFACE ${CMAKE_SOURCE_DIR}/src/Memory)

add_library(BridgeUtils INTERFACE)
target_include_directories(BridgeUtils INTERFACE ${CMAKE_SOURCE_DIR}/src/BridgeUtils)
target_link_libraries(BridgeUtils INTERFACE OBS::libobs Logger)
target_sources(
  BridgeUtils
  PRIVATE
    src/BridgeUtils/AsyncTextureReader.hpp
    src/BridgeUtils/GsUnique.hpp
    src/BridgeUtils/ObsLogger.hpp
    src/BridgeUtils/ObsUnique.hpp
)

add_library(TaskQueue INTERFACE)
target_include_directories(TaskQueue INTERFACE ${CMAKE_SOURCE_DIR}/src/TaskQueue)
target_link_libraries(TaskQueue INTERFACE BridgeUtils)
target_sources(TaskQueue PRIVATE src/TaskQueue/ThrottledTaskQueue.hpp)

add_library(UpdateChecker INTERFACE)
target_include_directories(UpdateChecker INTERFACE ${CMAKE_SOURCE_DIR}/src/UpdateChecker)
target_link_libraries(UpdateChecker INTERFACE CURL::libcurl)
target_sources(UpdateChecker PRIVATE src/UpdateChecker/UpdateChecker.hpp)

add_library(SelfieSegmenter STATIC)
target_sources(SelfieSegmenter PRIVATE src/SelfieSegmenter/ShapeConverter.cpp)
target_include_directories(SelfieSegmenter PUBLIC ${CMAKE_SOURCE_DIR}/src/SelfieSegmenter)
target_link_libraries(SelfieSegmenter PUBLIC Memory ncnn::ncnn)

target_compile_definitions(
  ${CMAKE_PROJECT_NAME}
  PRIVATE PLUGIN_NAME="${CMAKE_PROJECT_NAME}" PLUGIN_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/Core/DebugWindow.cpp
    src/Core/MainPluginContext_c.cpp
    src/Core/MainPluginContext.cpp
    src/Core/RenderingContext.cpp
    src/plugin-main.c
)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PUBLIC OBS::libobs BridgeUtils SelfieSegmenter UpdateChecker TaskQueue
)
if(Backward_FOUND)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Backward::Backward)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HAVE_BACKWARD)
endif()
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC NOMINMAX)

if(APPLE)
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,-exported_symbols_list,${CMAKE_SOURCE_DIR}/cmake/macos/exported_symbols_list.txt"
  )
elseif(MSVC)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/cmake/windows/export_symbols.def")
else()
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,--version-script,${CMAKE_SOURCE_DIR}/cmake/linux/version_script.map"
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(BUILD_TESTING)
  enable_testing()

  add_subdirectory(vendor/googletest)

  add_library(stb::stb INTERFACE IMPORTED)
  target_include_directories(stb::stb INTERFACE ${CMAKE_SOURCE_DIR}/vendor/stb)

  add_subdirectory(tests)
endif()
