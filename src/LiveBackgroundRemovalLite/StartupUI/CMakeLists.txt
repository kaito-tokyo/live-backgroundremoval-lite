add_library(${CMAKE_PROJECT_NAME}_StartupUI STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_StartupUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_StartupUI
  PUBLIC
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Widgets
)
qt_add_resources(STARTUPUI_RESOURCES "plugin_resources"
    PREFIX "/lbrl-lite-startupui"
    FILES
    header.png
)
target_sources(
  ${CMAKE_PROJECT_NAME}_StartupUI
  PRIVATE
    StartupController.cpp
    StartupController.hpp
    ${STARTUPUI_RESOURCES}
)
target_compile_options(
  ${CMAKE_PROJECT_NAME}_StartupUI
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
set_target_properties(
  ${CMAKE_PROJECT_NAME}_StartupUI
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
)
