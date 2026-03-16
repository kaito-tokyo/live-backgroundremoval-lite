# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# Read buildspec.json for project metadata
block(
  PROPAGATE PLUGIN_NAME PLUGIN_VERSION PLUGIN_AUTHOR PLUGIN_EMAIL PREBUILT_VERSION QT6_VERSION MACOS_BUNDLEID
)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" BUILDSPEC_CONTENT)
  string(JSON PLUGIN_NAME GET "${BUILDSPEC_CONTENT}" name)
  string(JSON PLUGIN_VERSION GET "${BUILDSPEC_CONTENT}" version)
  string(JSON PLUGIN_AUTHOR GET "${BUILDSPEC_CONTENT}" author)
  string(JSON PLUGIN_EMAIL GET "${BUILDSPEC_CONTENT}" email)
  string(JSON PREBUILT_VERSION GET "${BUILDSPEC_CONTENT}" dependencies prebuilt version)
  string(JSON QT6_VERSION GET "${BUILDSPEC_CONTENT}" dependencies qt6 version)
  string(JSON MACOS_BUNDLEID GET "${BUILDSPEC_CONTENT}" platformConfig macos bundleId)
endblock()

if(DEFINED ENV{GITHUB_RUN_ID})
  set(PLUGIN_BUILD_NUMBER "$ENV{GITHUB_RUN_ID}")
else()
  set(PLUGIN_BUILD_NUMBER "1")
endif()

project("${PLUGIN_NAME}" VERSION "${PLUGIN_VERSION}" LANGUAGES C CXX)

set(PLUGIN_VERSION_MAJOR "${CMAKE_PROJECT_VERSION_MAJOR}")
set(PLUGIN_VERSION_MINOR "${CMAKE_PROJECT_VERSION_MINOR}")
set(PLUGIN_VERSION_PATCH "${CMAKE_PROJECT_VERSION_PATCH}")

# Build options
set(VCPKG_TARGET_TRIPLET "" CACHE STRING "Vcpkg target triplet to use")
option(BUILD_TESTING "Build test cases" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)

# Basic configuration
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if(MSVC)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(APPLE)
  add_compile_options(-fno-omit-frame-pointer -fstack-protector-strong)
else()
  add_compile_options(-fno-omit-frame-pointer -fstack-protector-strong)
endif()

# Include paths
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common")
if(MSVC)
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows")
  list(
    PREPEND
    CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-qt6-${QT6_VERSION}-x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${PREBUILT_VERSION}-x64"
  )
elseif(APPLE)
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
  list(
    PREPEND
    CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/Frameworks"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-qt6-${QT6_VERSION}-universal"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${PREBUILT_VERSION}-universal"
  )
else()
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/linux")
endif()

if(VCPKG_TARGET_TRIPLET STREQUAL "")
  message(STATUS "Build without vcpkg")
else()
  message(STATUS "Build with vcpkg with target triplet: ${VCPKG_TARGET_TRIPLET}")
  list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.deps_vendor/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
endif()

if(ENABLE_ASAN)
  if(MSVC)
    message(
      WARNING
      "ENABLE_ASAN is enabled, but ASan flags are not applied for MSVC due to limited support and incompatible flag syntax."
    )
  else()
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
endif()

if(ENABLE_UBSAN)
  if(MSVC)
    message(
      WARNING
      "ENABLE_UBSAN is enabled, but UBSan flags are not applied for MSVC due to limited support and incompatible flag syntax."
    )
  else()
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
  endif()
endif()

# Common initialization
include(bootstrap)
include(compilerconfig)
include(defaults)
include(helpers)

# Dependent packages

## PkgConfig
find_package(PkgConfig QUIET)

## OBS and Qt
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

## CURL
find_package(CURL CONFIG QUIET)

if(NOT CURL_FOUND AND PkgConfig_FOUND)
  pkg_check_modules(PC_CURL libcurl IMPORTED_TARGET)
  if(PC_CURL_FOUND)
    add_library(CURL::libcurl ALIAS PkgConfig::PC_CURL)
    set(CURL_FOUND TRUE)
  endif()
endif()

if(NOT CURL_FOUND)
  message(FATAL_ERROR "CURL not found via CMake or pkg-config.")
endif()

## fmt
find_package(fmt CONFIG QUIET)

if(NOT fmt_FOUND AND PkgConfig_FOUND)
  pkg_check_modules(PC_FMT fmt IMPORTED_TARGET)
  if(PC_FMT_FOUND)
    add_library(fmt::fmt ALIAS PkgConfig::PC_FMT)
    set(fmt_FOUND TRUE)
  endif()
endif()

if(NOT fmt_FOUND)
  message(FATAL_ERROR "fmt not found via CMake or pkg-config.")
endif()

## ncnn
find_package(ncnn CONFIG QUIET)

if(NOT ncnn_FOUND AND PkgConfig_FOUND)
  pkg_check_modules(PC_NCNN ncnn IMPORTED_TARGET)
  if(PC_NCNN_FOUND)
    add_library(ncnn::ncnn ALIAS PkgConfig::PC_NCNN)
    set(ncnn_FOUND TRUE)
  endif()
endif()

if(NOT ncnn_FOUND)
  message(FATAL_ERROR "ncnn not found via CMake or pkg-config.")
endif()

if(NOT TARGET ncnn::ncnn)
  add_library(ncnn::ncnn ALIAS ncnn)
endif()

## jthread
if(APPLE)
  set(JTHREAD_LIBRARIES jthread::jthread)
  add_subdirectory(vendor/josuttis-jthread)
endif()

# Plugin targets

add_subdirectory(src)

add_library(${CMAKE_PROJECT_NAME} MODULE)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    CURL::libcurl
    OBS::libobs
    ObsBridgeUtils
    ${CMAKE_PROJECT_NAME}_Global
    ${CMAKE_PROJECT_NAME}_MainFilter
    ${CMAKE_PROJECT_NAME}_StartupUI
    Qt6::Core
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp src/resources/resources.qrc)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTORCC ON)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PLUGIN_VERSION="${PLUGIN_VERSION}")

# Exported symbols
if(APPLE)
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/src/exported_symbols_macos.txt"
  )
elseif(MSVC)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/exported_symbols_windows.def")
else()
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/src/exported_symbols_linux.map" "-Wl,-Bsymbolic"
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

if(BUILD_TESTING)
  enable_testing()

  add_subdirectory(vendor/googletest EXCLUDE_FROM_ALL)

  add_library(stb::stb INTERFACE IMPORTED)
  target_include_directories(stb::stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb)

  add_subdirectory(tests)
endif()

# CPack configuration
set(CPACK_ARCHIVE_THREADS 0)
set(CPACK_DEBIAN_COMPRESSION_LEVEL 19)
set(CPACK_DEBIAN_COMPRESSION_TYPE zstd)
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE TRUE)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS TRUE)
set(CPACK_GENERATOR "DEB")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/release")
set(CPACK_PACKAGE_CONTACT "${PLUGIN_AUTHOR} <${PLUGIN_EMAIL}>")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_C_LIBRARY_ARCHITECTURE}")
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/GPL-3.0-or-later.txt")
set(CPACK_SET_DESTDIR TRUE)
set(CPACK_VERBATIM_VARIABLES TRUE)
set(
  CPACK_SOURCE_IGNORE_FILES
  ".deps_vendor/"
  ".git/"
  "build_x86_64/"
  "build_x64/"
  "build_macos/"
  "release/"
  "vcpkg_installed/"
)

# Include CPack after setting variables
include(CPack)
