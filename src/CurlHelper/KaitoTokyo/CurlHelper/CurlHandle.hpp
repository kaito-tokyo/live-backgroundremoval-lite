// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

class CurlHandle {
	struct CurlDeleter {
		void operator()(CURL *ptr) const { curl_easy_cleanup(ptr); }
	};

public:
	CurlHandle() : curl_(curl_easy_init())
	{
		if (!curl_) {
			throw std::runtime_error("CurlEasyInitError(CurlHandle)");
		}
	}

	~CurlHandle() noexcept = default;

	CurlHandle(const CurlHandle &) = delete;
	CurlHandle &operator=(const CurlHandle &) = delete;
	CurlHandle(CurlHandle &&) = delete;
	CurlHandle &operator=(CurlHandle &&) = delete;

	[[nodiscard]]
	CURL *getRaw() const
	{
		return curl_.get();
	}

private:
	const std::unique_ptr<CURL, CurlDeleter> curl_;
};

} // namespace KaitoTokyo::CurlHelper
