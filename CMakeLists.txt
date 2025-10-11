cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

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

set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed")
list(PREPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

option(USE_PKGCONFIG "Use pkg-config to find dependencies" OFF)

if(USE_PKGCONFIG)
  find_package(PkgConfig REQUIRED)

  # --- CURL ---
  pkg_check_modules(PC_CURL REQUIRED libcurl)
  add_library(CURL::libcurl INTERFACE IMPORTED)
  target_link_libraries(CURL::libcurl INTERFACE ${PC_CURL_LIBRARIES})
  target_include_directories(CURL::libcurl INTERFACE ${PC_CURL_INCLUDE_DIRS})
  target_compile_definitions(CURL::libcurl INTERFACE ${PC_CURL_CFLAGS_OTHER})

  # --- fmt ---
  pkg_check_modules(PC_FMT REQUIRED fmt)
  add_library(fmt::fmt INTERFACE IMPORTED)
  target_link_libraries(fmt::fmt INTERFACE ${PC_FMT_LIBRARIES})
  target_include_directories(fmt::fmt INTERFACE ${PC_FMT_INCLUDE_DIRS})
  target_compile_definitions(fmt::fmt INTERFACE ${PC_FMT_CFLAGS_OTHER})

  # --- ncnn ---
  pkg_check_modules(PC_NCNN REQUIRED ncnn)
  add_library(ncnn INTERFACE IMPORTED)
  target_link_libraries(ncnn INTERFACE ${PC_NCNN_LIBRARIES})
  target_include_directories(ncnn INTERFACE ${PC_NCNN_INCLUDE_DIRS})
  target_compile_definitions(ncnn INTERFACE ${PC_NCNN_CFLAGS_OTHER})

  # --- OpenCV ---
  pkg_check_modules(PC_OPENCV REQUIRED opencv4)
  add_library(opencv_core INTERFACE IMPORTED)
  target_link_libraries(opencv_core INTERFACE opencv_core)
  target_include_directories(opencv_core INTERFACE ${PC_OPENCV_INCLUDE_DIRS})
  target_compile_definitions(opencv_core INTERFACE ${PC_OPENCV_CFLAGS_OTHER})
  add_library(opencv_imgproc INTERFACE IMPORTED)
  target_link_libraries(opencv_imgproc INTERFACE opencv_imgproc)
  add_library(opencv_imgcodecs INTERFACE IMPORTED)
  target_link_libraries(opencv_imgcodecs INTERFACE opencv_imgcodecs)
else()
  find_package(Backward CONFIG)
  find_package(CURL CONFIG REQUIRED)
  find_package(fmt CONFIG REQUIRED)
  find_package(ncnn CONFIG REQUIRED)
  find_package(OpenCV CONFIG REQUIRED)
endif()

add_library(SelfieSegmenter STATIC)
target_sources(SelfieSegmenter PRIVATE src/SelfieSegmenter/SelfieSegmenter.cpp)
target_include_directories(SelfieSegmenter PUBLIC ${CMAKE_SOURCE_DIR}/src/SelfieSegmenter)
target_link_libraries(SelfieSegmenter PUBLIC ncnn)

add_library(UpdateChecker INTERFACE)
target_include_directories(UpdateChecker INTERFACE ${CMAKE_SOURCE_DIR}/src/UpdateChecker)
target_link_libraries(UpdateChecker INTERFACE CURL::libcurl)

add_library(BridgeUtils INTERFACE)
target_include_directories(BridgeUtils INTERFACE ${CMAKE_SOURCE_DIR}/src/BridgeUtils)
target_link_libraries(BridgeUtils INTERFACE OBS::libobs fmt::fmt)

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
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC OBS::libobs BridgeUtils SelfieSegmenter UpdateChecker)
if(Backward_FOUND)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Backward::Backward)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HAVE_BACKWARD)
endif()
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC NOMINMAX)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(BUILD_TESTING)
  enable_testing()

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/stb")

  find_package(GTest CONFIG REQUIRED)

  add_subdirectory(tests)
endif()
