# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(Memory INTERFACE)
target_include_directories(Memory INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Memory PRIVATE KaitoTokyo/Memory/AlignedAllocator.hpp KaitoTokyo/Memory/MemoryBlockPool.hpp)
