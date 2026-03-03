// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

class CurlUrlHandle {
	struct CurlUrlDeleter {
		void operator()(CURLU *ptr) const { curl_url_cleanup(ptr); }
	};

public:
	CurlUrlHandle() : handle_(curl_url())
	{
		if (!handle_) {
			throw std::runtime_error("CurlUrlInitError(CurlUrlHandle)");
		}
	};

	~CurlUrlHandle() noexcept = default;

	CurlUrlHandle(const CurlUrlHandle &) = delete;
	CurlUrlHandle &operator=(const CurlUrlHandle &) = delete;
	CurlUrlHandle(CurlUrlHandle &&) = delete;
	CurlUrlHandle &operator=(CurlUrlHandle &&) = delete;

	void setUrl(const char *url)
	{
		CURLUcode uc = curl_url_set(handle_.get(), CURLUPART_URL, url, 0);
		if (uc != CURLUE_OK)
			throw std::runtime_error("URLParseError(setUrl)");
	}

	void appendQuery(const char *query)
	{
		CURLUcode uc = curl_url_set(handle_.get(), CURLUPART_QUERY, query, CURLU_APPENDQUERY);
		if (uc != CURLUE_OK)
			throw std::runtime_error("QueryAppendError(appendQuery)");
	}

	[[nodiscard]]
	auto c_str() const
	{
		char *urlStr = nullptr;
		CURLUcode uc = curl_url_get(handle_.get(), CURLUPART_URL, &urlStr, 0);
		if (uc != CURLUE_OK || !urlStr) {
			throw std::runtime_error("GetUrlError(c_str)");
		}
		return std::unique_ptr<char, decltype(&curl_free)>(urlStr, curl_free);
	}

private:
	std::unique_ptr<CURLU, CurlUrlDeleter> handle_;
};

} // namespace KaitoTokyo::CurlHelper
