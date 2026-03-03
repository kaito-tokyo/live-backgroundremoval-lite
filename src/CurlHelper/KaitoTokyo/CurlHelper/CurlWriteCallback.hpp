// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

template<typename T> concept SingleByte = sizeof(T) == 1;

template<SingleByte T>
inline std::size_t CurlVectorWriteCallback(void *contents, std::size_t size, std::size_t nmemb, void *userp) noexcept
{
	if (size != 0 && nmemb > (std::numeric_limits<std::size_t>::max() / size)) {
		return CURL_WRITEFUNC_ERROR;
	}

	std::size_t totalSize = size * nmemb;

	try {
		auto *vec = static_cast<std::vector<T> *>(userp);

		const auto *start = static_cast<const T *>(contents);
		const auto *end = start + totalSize;

		vec->insert(vec->end(), start, end);
	} catch (...) {
		return CURL_WRITEFUNC_ERROR;
	}

	return totalSize;
}

inline std::size_t CurlByteVectorWriteCallback(void *contents, std::size_t size, std::size_t nmemb,
					       void *userp) noexcept
{
	return CurlVectorWriteCallback<std::byte>(contents, size, nmemb, userp);
}

inline std::size_t CurlCharVectorWriteCallback(void *contents, std::size_t size, std::size_t nmemb,
					       void *userp) noexcept
{
	return CurlVectorWriteCallback<char>(contents, size, nmemb, userp);
}

inline std::size_t CurlUint8VectorWriteCallback(void *contents, std::size_t size, std::size_t nmemb,
						void *userp) noexcept
{
	return CurlVectorWriteCallback<std::uint8_t>(contents, size, nmemb, userp);
}

} // namespace KaitoTokyo::CurlHelper
