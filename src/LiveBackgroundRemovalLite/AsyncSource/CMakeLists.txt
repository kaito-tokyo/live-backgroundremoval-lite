add_library(${CMAKE_PROJECT_NAME}_AsyncSource STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_AsyncSource PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_AsyncSource
  PUBLIC
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
    ObsBridgeUtils
    SelfieSegmenter
    TaskQueue
    ${CMAKE_PROJECT_NAME}_Global
    ${CMAKE_PROJECT_NAME}_MainFilter
)
target_sources(
  ${CMAKE_PROJECT_NAME}_AsyncSource
  PRIVATE AsyncSourceContext.cpp AsyncSourceContext.hpp AsyncSourceInfo.cpp AsyncSourceInfo.hpp
)
target_compile_options(
  ${CMAKE_PROJECT_NAME}_AsyncSource
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
