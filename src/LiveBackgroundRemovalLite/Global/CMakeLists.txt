add_library(${CMAKE_PROJECT_NAME}_Global STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_Global PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Global
  PUBLIC ${JTHREAD_LIBRARIES} Async Logger
  PRIVATE Qt6::Widgets Qt6::Core CURL::libcurl BridgeUtils CurlHelper
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Global
  PRIVATE
    GlobalContext.cpp
    GlobalContext.hpp
    PluginConfig.cpp
    PluginConfig.hpp
    PluginConfigDialog.cpp
    PluginConfigDialog.hpp
    licenses.qrc
)
set_target_properties(${CMAKE_PROJECT_NAME}_Global PROPERTIES AUTOMOC ON AUTORCC ON)
