# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(Async INTERFACE)
target_include_directories(Async INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Async INTERFACE KaitoTokyo/Async/Channel.hpp KaitoTokyo/Async/Join.hpp KaitoTokyo/Async/Task.hpp)
