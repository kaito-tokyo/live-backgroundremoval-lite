# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(SimpleJsonReader INTERFACE)
target_sources(SimpleJsonReader PUBLIC FILE_SET HEADERS BASE_DIRS . FILES KaitoTokyo/SimpleJsonReader/SimpleJsonReader.hpp)
