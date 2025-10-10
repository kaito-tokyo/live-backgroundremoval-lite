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

  pkg_check_modules(PC_CURL REQUIRED libcurl)
  add_library(CURL::libcurl INTERFACE IMPORTED GLOBAL)
  set_target_properties(
    CURL::libcurl
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${PC_CURL_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${PC_CURL_LIBRARIES}"
      INTERFACE_COMPILE_DEFINITIONS "${PC_CURL_CFLAGS_OTHER}"
  )

  pkg_check_modules(PC_FMT REQUIRED fmt)
  add_library(fmt::fmt INTERFACE IMPORTED GLOBAL)
  set_target_properties(
    fmt::fmt
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${PC_FMT_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${PC_FMT_LIBRARIES}"
      INTERFACE_COMPILE_DEFINITIONS "${PC_FMT_CFLAGS_OTHER}"
  )

  pkg_check_modules(PC_NCNN REQUIRED ncnn)
  add_library(ncnn INTERFACE IMPORTED GLOBAL)
  set_target_properties(
    ncnn
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${PC_NCNN_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${PC_NCNN_LIBRARIES}"
      INTERFACE_COMPILE_DEFINITIONS "${PC_NCNN_CFLAGS_OTHER}"
  )

  pkg_check_modules(PC_OPENCV REQUIRED opencv4)
  add_library(opencv_all INTERFACE IMPORTED GLOBAL)
  set_target_properties(
    opencv_all
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${PC_OPENCV_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${PC_OPENCV_LIBRARIES}"
      INTERFACE_COMPILE_DEFINITIONS "${PC_OPENCV_CFLAGS_OTHER}"
  )
  set(OPENCV_LIBS opencv_all)
else()
  find_package(Backward CONFIG)
  find_package(CURL CONFIG REQUIRED)
  find_package(fmt CONFIG REQUIRED)
  find_package(ncnn CONFIG REQUIRED)
  find_package(OpenCV CONFIG REQUIRED)
  set(OPENCV_LIBS opencv_core opencv_imgproc opencv_imgcodecs)
endif()

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
    src/SelfieSegmenter/SelfieSegmenter.cpp
    src/plugin-main.c
)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PUBLIC OBS::libobs CURL::libcurl fmt::fmt ncnn ${OPENCV_LIBS}
)
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
  find_package(Stb MODULE REQUIRED)

  add_subdirectory(tests)
endif()
