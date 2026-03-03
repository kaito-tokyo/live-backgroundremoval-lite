# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(TaskQueue INTERFACE)
target_include_directories(TaskQueue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TaskQueue INTERFACE Logger)
target_sources(TaskQueue PRIVATE KaitoTokyo/TaskQueue/ThrottledTaskQueue.hpp)
