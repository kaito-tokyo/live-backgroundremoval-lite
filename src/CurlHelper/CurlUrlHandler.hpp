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

#include <memory>
#include <stdexcept>
#include <string>

namespace KaitoTokyo::CurlHelper {

class CurlUrlHandle {
public:
	CurlUrlHandle() : handle_(curl_url())
	{
		if (!handle_) {
			throw std::runtime_error("InitError(CurlUrlHandle)");
		}
	}

	~CurlUrlHandle() noexcept
	{
		if (handle_) {
			curl_url_cleanup(handle_);
		}
	}

	CurlUrlHandle(const CurlUrlHandle &) = delete;
	CurlUrlHandle &operator=(const CurlUrlHandle &) = delete;

	void setUrl(const char *url)
	{
		CURLUcode uc = curl_url_set(handle_, CURLUPART_URL, url, 0);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("URLParseError(CurlUrlHandle):" + std::string(url));
		}
	}

	void appendQuery(const char *query)
	{
		CURLUcode uc = curl_url_set(handle_, CURLUPART_QUERY, query, CURLU_APPENDQUERY);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("QueryAppendError(CurlUrlHandle):" + std::string(query));
		}
	}

	std::unique_ptr<char, decltype(&curl_free)> c_str() const
	{
		char *urlStr = nullptr;
		CURLUcode uc = curl_url_get(handle_, CURLUPART_URL, &urlStr, 0);
		if (uc != CURLUE_OK || !urlStr) {
			throw std::runtime_error("GetUrlError(CurlUrlHandle)");
		}
		return std::unique_ptr<char, decltype(&curl_free)>(urlStr, curl_free);
	}

private:
	CURLU *const handle_;
};

} // namespace KaitoTokyo::CurlHelper
