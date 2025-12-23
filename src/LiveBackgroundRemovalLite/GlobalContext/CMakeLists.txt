add_library(${CMAKE_PROJECT_NAME}_GlobalContext INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_GlobalContext INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${CMAKE_PROJECT_NAME}_GlobalContext PRIVATE GlobalContext.hpp)
