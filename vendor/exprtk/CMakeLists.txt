# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(exprtk::exprtk INTERFACE IMPORTED GLOBAL)
target_include_directories(exprtk::exprtk INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
