# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

add_library(PluginManager OBJECT UpdateChecker.cpp)
target_link_libraries(PluginManager PRIVATE CURL::libcurl wolfssl::wolfssl CurlHelper)
target_sources(PluginManager PUBLIC FILE_SET HEADERS BASE_DIRS . FILES KaitoTokyo/PluginManager/UpdateChecker.hpp)
