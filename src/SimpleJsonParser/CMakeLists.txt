# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(SimpleJsonParser INTERFACE)
target_sources(SimpleJsonParser PUBLIC FILE_SET HEADERS BASE_DIRS . FILES KaitoTokyo/SimpleJsonParser/SimpleJsonParser.hpp)
