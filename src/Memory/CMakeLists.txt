add_library(Memory INTERFACE)
target_include_directories(Memory INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Memory PRIVATE AlignedMemoryResource.hpp MemoryBlockPool.hpp)
