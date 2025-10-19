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

option(BUILD_TESTING "Build tests" OFF)
option(BUILD_WITH_HOMEBREW "Build with Homebrew on macOS" OFF)
option(USE_PKGCONFIG "Use pkg-config to find dependencies" OFF)

if(VCPKG_TARGET_TRIPLET)
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

  find_package(Backward CONFIG)

  find_package(CURL CONFIG REQUIRED)
  find_package(fmt CONFIG REQUIRED)
  find_package(ncnn CONFIG REQUIRED)
  find_package(OpenCV CONFIG REQUIRED)

  add_library(ncnn::ncnn ALIAS ncnn)
  add_library(OpenCV::opencv_core ALIAS opencv_core)
  add_library(OpenCV::opencv_imgproc ALIAS opencv_imgproc)
  add_library(OpenCV::opencv_imgcodecs ALIAS opencv_imgcodecs)
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

  # --- OpenCV ---
  pkg_check_modules(PC_OPENCV REQUIRED opencv4)
  add_library(OpenCV::opencv_core INTERFACE IMPORTED)
  target_link_libraries(OpenCV::opencv_core INTERFACE opencv_core)
  target_include_directories(OpenCV::opencv_core INTERFACE ${PC_OPENCV_INCLUDE_DIRS})
  target_compile_definitions(OpenCV::opencv_core INTERFACE ${PC_OPENCV_CFLAGS_OTHER})
  target_link_directories(OpenCV::opencv_core INTERFACE ${PC_OPENCV_LIBRARY_DIRS})
  add_library(OpenCV::opencv_imgproc INTERFACE IMPORTED)
  target_link_libraries(OpenCV::opencv_imgproc INTERFACE opencv_imgproc)
  target_link_directories(OpenCV::opencv_imgproc INTERFACE ${PC_OPENCV_LIBRARY_DIRS})
  add_library(OpenCV::opencv_imgcodecs INTERFACE IMPORTED)
  target_link_libraries(OpenCV::opencv_imgcodecs INTERFACE opencv_imgcodecs)
  target_link_directories(OpenCV::opencv_imgcodecs INTERFACE ${PC_OPENCV_LIBRARY_DIRS})
elseif(APPLE AND BUILD_WITH_HOMEBREW)
  list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/libomp)
  list(APPEND CMAKE_PREFIX_PATH /opt/homebrew)
  list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/libomp)
  list(APPEND CMAKE_PREFIX_PATH /usr/local)

  find_package(CURL MODULE REQUIRED)
  find_package(fmt CONFIG REQUIRED)
  find_package(ncnn CONFIG REQUIRED)
  find_package(OpenCV CONFIG REQUIRED)

  add_library(ncnn::ncnn ALIAS ncnn)
  add_library(OpenCV::opencv_core ALIAS opencv_core)
  add_library(OpenCV::opencv_imgproc ALIAS opencv_imgproc)
  add_library(OpenCV::opencv_imgcodecs ALIAS opencv_imgcodecs)
else()
  message(FATAL_ERROR "Either USE_PKGCONFIG or VCPKG_TARGET_TRIPLET must be set.")
endif()

if(APPLE)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_default_derived_data_path "${CMAKE_BINARY_DIR}/DerivedData/Build/Products/Debug")
  else()
    set(_default_derived_data_path "${CMAKE_BINARY_DIR}/DerivedData/Build/Products/Release")
  endif()
  set(DERIVED_DATA_PATH "${_default_derived_data_path}" CACHE PATH "Path to Xcode DerivedData build products")

  find_library(COREML_SELFIE_SEGMENTER_FRAMEWORK_PATH NAMES CoreMLSelfieSegmenter HINTS ${DERIVED_DATA_PATH})

  if(COREML_SELFIE_SEGMENTER_FRAMEWORK_PATH)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${COREML_SELFIE_SEGMENTER_FRAMEWORK_PATH}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HAVE_COREML_SELFIE_SEGMENTER)

    set(PLUGIN_BUNDLE_DIR "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/../..")
    set(FRAMEWORKS_DIR "${PLUGIN_BUNDLE_DIR}/Contents/Frameworks")
    add_custom_command(
      TARGET ${CMAKE_PROJECT_NAME}
      POST_BUILD
      COMMAND mkdir -p "${FRAMEWORKS_DIR}"
      COMMAND rm -rf "${FRAMEWORKS_DIR}/CoreMLSelfieSegmenter.framework"
      COMMAND cp -R "${COREML_SELFIE_SEGMENTER_FRAMEWORK_PATH}" "${FRAMEWORKS_DIR}/CoreMLSelfieSegmenter.framework"
      COMMENT "Copying CoreMLSelfieSegmenter.framework to plugin bundle Frameworks directory (preserving symlinks)"
    )
  endif()
endif()

add_library(SelfieSegmenter STATIC)
target_sources(
  SelfieSegmenter
  PRIVATE src/SelfieSegmenter/ShapeConverter.cpp src/SelfieSegmenter/NcnnSelfieSegmenter.cpp
)
target_include_directories(SelfieSegmenter PUBLIC ${CMAKE_SOURCE_DIR}/src/SelfieSegmenter)
target_link_libraries(SelfieSegmenter PUBLIC ncnn::ncnn)

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

set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@loader_path/../Frameworks"
)

if(BUILD_TESTING)
  enable_testing()

  find_package(GTest CONFIG REQUIRED)

  add_subdirectory(tests)
endif()
