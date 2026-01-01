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
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

template<typename T> concept CurlReaderInputStreamLike = requires(T & t, char *ptr, std::streamsize n)
{
	t.read(ptr, n);
	{
		t.gcount()
	} -> std::convertible_to<std::streamsize>;
};

template<CurlReaderInputStreamLike StreamT>
inline std::size_t CurlStreamReadCallback(char *buffer, std::size_t size, std::size_t nmemb, void *userp) noexcept
{
	if (size != 0 && nmemb > (std::numeric_limits<std::size_t>::max() / size)) {
		return CURL_READFUNC_ABORT;
	}

	std::size_t totalSize = size * nmemb;

	if (totalSize > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
		return CURL_READFUNC_ABORT;
	}

	std::streamsize bufferSize = static_cast<std::streamsize>(totalSize);
	if (bufferSize == 0)
		return 0;

	try {
		auto *stream = static_cast<StreamT *>(userp);

		if (!stream || !(*stream)) {
			return 0;
		}

		stream->read(buffer, bufferSize);
		return static_cast<std::size_t>(stream->gcount());
	} catch (...) {
		return CURL_READFUNC_ABORT;
	}
}

inline std::size_t CurlIfstreamReadCallback(char *buffer, std::size_t size, std::size_t nmemb, void *userp) noexcept
{
	return CurlStreamReadCallback<std::ifstream>(buffer, size, nmemb, userp);
}

inline std::size_t CurlIstreamReadCallback(char *buffer, std::size_t size, std::size_t nmemb, void *userp) noexcept
{
	return CurlStreamReadCallback<std::istream>(buffer, size, nmemb, userp);
}

inline std::size_t CurlStringStreamReadCallback(char *buffer, std::size_t size, std::size_t nmemb, void *userp) noexcept
{
	return CurlStreamReadCallback<std::istringstream>(buffer, size, nmemb, userp);
}

} // namespace KaitoTokyo::CurlHelper
