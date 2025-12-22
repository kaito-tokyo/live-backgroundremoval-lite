add_library(${CMAKE_PROJECT_NAME}_Core STATIC)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Core
  PUBLIC OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Widgets BridgeUtils SelfieSegmenter UpdateChecker TaskQueue
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Core
  PRIVATE
  DebugWindow.cpp
  DebugWindow.hpp
  MainEffect.hpp
  MainPluginContext_c.cpp
  MainPluginContext.cpp
  MainPluginContext.h
  PluginConfig.hpp
  PluginProperty.hpp
  RenderingContext.cpp
  RenderingContext.hpp
)
target_compile_options(
  ${CMAKE_PROJECT_NAME}_Core
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
set_target_properties(
  ${CMAKE_PROJECT_NAME}_Core
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
)
