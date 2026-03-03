# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(Logger INTERFACE)
target_include_directories(Logger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(
  Logger
  PRIVATE
    KaitoTokyo/Logger/ILogger.hpp
    KaitoTokyo/Logger/NullLogger.hpp
    KaitoTokyo/Logger/MultiLogger.hpp
    KaitoTokyo/Logger/PrintLogger.hpp
)
