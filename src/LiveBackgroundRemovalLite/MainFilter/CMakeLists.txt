<<<<<<<< HEAD:src/LiveBackgroundRemovalLite/MainFilter/CMakeLists.txt
add_library(${CMAKE_PROJECT_NAME}_MainFilter STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_MainFilter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_MainFilter
  PUBLIC OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Widgets BridgeUtils SelfieSegmenter TaskQueue ${CMAKE_PROJECT_NAME}_Global
)
target_sources(
  ${CMAKE_PROJECT_NAME}_MainFilter
========
add_library(${CMAKE_PROJECT_NAME}_Filter STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_Filter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Filter
  PUBLIC OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Widgets BridgeUtils SelfieSegmenter UpdateChecker TaskQueue
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Filter
>>>>>>>> main:src/LiveBackgroundRemovalLite/Filter/CMakeLists.txt
  PRIVATE
    DebugWindow.cpp
    DebugWindow.hpp
    MainEffect.hpp
    MainFilterContext.cpp
    MainFilterContext.hpp
    MainFilterInfo.cpp
    MainFilterInfo.hpp
    PluginConfig.hpp
    PluginProperty.hpp
    RenderingContext.cpp
    RenderingContext.hpp
)
target_compile_options(
<<<<<<<< HEAD:src/LiveBackgroundRemovalLite/MainFilter/CMakeLists.txt
  ${CMAKE_PROJECT_NAME}_MainFilter
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
set_target_properties(
  ${CMAKE_PROJECT_NAME}_MainFilter
========
  ${CMAKE_PROJECT_NAME}_Filter
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
set_target_properties(
  ${CMAKE_PROJECT_NAME}_Filter
>>>>>>>> main:src/LiveBackgroundRemovalLite/Filter/CMakeLists.txt
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
)
