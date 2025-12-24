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
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

if(VCPKG_TARGET_TRIPLET)
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

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

if(APPLE)
  set(JTHREAD_LIBRARIES jthread::jthread)
  add_subdirectory(vendor/josuttis-jthread)
endif()

add_subdirectory(src)

add_library(${CMAKE_PROJECT_NAME} MODULE)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    CURL::libcurl
    OBS::libobs
    BridgeUtils
    ${CMAKE_PROJECT_NAME}_Global
    ${CMAKE_PROJECT_NAME}_MainFilter
    ${CMAKE_PROJECT_NAME}_StartupUI
    Qt6::Core
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp src/resources/resources.qrc)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTORCC ON)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PLUGIN_VERSION="${PLUGIN_VERSION}")

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
    PRIVATE "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/src/exported_symbols_linux.map"
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
