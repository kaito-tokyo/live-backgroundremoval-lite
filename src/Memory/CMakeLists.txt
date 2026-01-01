add_library(Memory INTERFACE)
target_include_directories(Memory INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Memory PRIVATE KaitoTokyo/Memory/AlignedAllocator.hpp KaitoTokyo/Memory/MemoryBlockPool.hpp)
