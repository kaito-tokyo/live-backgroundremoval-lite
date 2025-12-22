add_library(Memory INTERFACE)
target_include_directories(Memory INTERFACE ${CMAKE_SOURCE_DIR}/src/Memory)
target_sources(Logger PRIVATE ILogger.hpp NullLogger.hpp)
