/*
 * KaitoTokyo CurlHelper Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <curl/curl.h>

#include <cstddef>
#include <limits>
#include <vector>

namespace KaitoTokyo::CurlHelper {

using CurlVectorWriterBuffer = std::vector<char>;

inline std::size_t CurlVectorWriter(void *contents, std::size_t size, std::size_t nmemb, void *userp) noexcept
{
	if (size != 0 && nmemb > (std::numeric_limits<std::size_t>::max() / size)) {
		return 0; // Signal error
	}

	std::size_t totalSize = size * nmemb;
	try {
		auto *vec = static_cast<CurlVectorWriterBuffer *>(userp);
		vec->insert(vec->end(), static_cast<char *>(contents), static_cast<char *>(contents) + totalSize);
	} catch (...) {
		return 0; // Signal error
	}
	return totalSize;
}

} // namespace KaitoTokyo::CurlHelper
