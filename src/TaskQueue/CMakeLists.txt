add_library(TaskQueue INTERFACE)
target_include_directories(TaskQueue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TaskQueue INTERFACE Logger)
target_sources(TaskQueue PRIVATE ThrottledTaskQueue.hpp)
