cmake_minimum_required(VERSION 3.28)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common")
include(init)

project(${PLUGIN_NAME} VERSION ${PLUGIN_VERSION} LANGUAGES C CXX)

option(BUILD_TESTING "Build test cases" ON)
option(USE_PKGCONFIG "Use pkg-config to find dependencies" OFF)

if(MSVC)
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows")
elseif(APPLE)
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
else()
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/linux")
endif()

include(compilerconfig)
include(defaults)
include(helpers)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(MSVC)
  list(
    PREPEND
    CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-qt6-${QT6_VERSION}-x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${PREBUILT_VERSION}-x64"
  )
elseif(APPLE)
  list(
    PREPEND
    CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/Frameworks"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-qt6-${QT6_VERSION}-universal"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${PREBUILT_VERSION}-universal"
  )
endif()

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 COMPONENTS Widgets Core REQUIRED)

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

add_library(
  ${CMAKE_PROJECT_NAME}
  MODULE
  src/Core/DebugWindow.cpp
  src/Core/MainPluginContext_c.cpp
  src/Core/MainPluginContext.cpp
  src/Core/RenderingContext.cpp
  src/plugin-main.c
)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PUBLIC OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Widgets BridgeUtils SelfieSegmenter UpdateChecker TaskQueue
)

if(Backward_FOUND)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Backward::Backward)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HAVE_BACKWARD)
endif()

if(MSVC)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/cmake/windows/resources/export_symbols.def")
elseif(APPLE)
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,-exported_symbols_list,${CMAKE_SOURCE_DIR}/cmake/macos/resources/exported_symbols_list.txt"
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@loader_path/../Frameworks"
  )
else()
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,--version-script,${CMAKE_SOURCE_DIR}/cmake/linux/resources/version_script.map"
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

foreach(target ${CMAKE_PROJECT_NAME})
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endforeach()

if(BUILD_TESTING)
  enable_testing()

  add_subdirectory(vendor/googletest EXCLUDE_FROM_ALL)

  add_library(stb::stb INTERFACE IMPORTED)
  target_include_directories(stb::stb INTERFACE ${CMAKE_SOURCE_DIR}/vendor/stb)

  add_subdirectory(tests)
endif()
