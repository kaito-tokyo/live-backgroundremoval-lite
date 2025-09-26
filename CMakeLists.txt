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

find_package(fmt CONFIG REQUIRED)
find_package(ncnn CONFIG REQUIRED)
find_package(semver CONFIG REQUIRED)

target_compile_definitions(
  ${CMAKE_PROJECT_NAME}
  PRIVATE PLUGIN_NAME="${CMAKE_PROJECT_NAME}" PLUGIN_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/MainPluginContext_c.cpp
    src/MainPluginContext.cpp
    src/RenderingContext.cpp
    src/DebugWindow.cpp
    src/plugin-main.c
)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PUBLIC OBS::libobs fmt::fmt ncnn semver::semver
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC NOMINMAX)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

add_library(${CMAKE_PROJECT_NAME}_header INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_header INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC NOMINMAX)

if(BUILD_TESTING)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()
