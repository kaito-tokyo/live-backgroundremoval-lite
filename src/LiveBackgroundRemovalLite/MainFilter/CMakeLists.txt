add_library(${CMAKE_PROJECT_NAME}_MainFilter STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_MainFilter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_MainFilter
  PUBLIC
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
    ObsBridgeUtils
    SelfieSegmenter
    TaskQueue
    ${CMAKE_PROJECT_NAME}_Global
)
target_sources(
  ${CMAKE_PROJECT_NAME}_MainFilter
  PRIVATE
    DebugWindow.cpp
    DebugWindow.hpp
    MainEffect.hpp
    MainFilterContext.cpp
    MainFilterContext.hpp
    MainFilterInfo.cpp
    MainFilterInfo.hpp
    PluginProperty.hpp
    RenderingContext.cpp
    RenderingContext.hpp
)
target_compile_options(
  ${CMAKE_PROJECT_NAME}_MainFilter
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
set_target_properties(
  ${CMAKE_PROJECT_NAME}_MainFilter
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
)
