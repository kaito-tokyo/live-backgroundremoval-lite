/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo CurlHelper Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
