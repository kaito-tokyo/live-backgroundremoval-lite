// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

template<typename T>
concept CurlReaderInputStreamLike = requires(T &t, char *ptr, std::streamsize n) {
	t.read(ptr, n);
	{ t.gcount() } -> std::convertible_to<std::streamsize>;
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
