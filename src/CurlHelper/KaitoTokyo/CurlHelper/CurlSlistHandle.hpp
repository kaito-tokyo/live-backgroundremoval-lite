// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <stdexcept>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

class CurlSlistHandle {
	struct CurlSlistDeleter {
		void operator()(curl_slist *ptr) const { curl_slist_free_all(ptr); }
	};

public:
	CurlSlistHandle() = default;

	~CurlSlistHandle() noexcept = default;

	CurlSlistHandle(const CurlSlistHandle &) = delete;
	CurlSlistHandle &operator=(const CurlSlistHandle &) = delete;
	CurlSlistHandle(CurlSlistHandle &&) = delete;
	CurlSlistHandle &operator=(CurlSlistHandle &&) = delete;

	void append(const char *str)
	{
		curl_slist *current = slist_.release();

		curl_slist *newSlist = curl_slist_append(current, str);

		if (newSlist) {
			slist_.reset(newSlist);
		} else {
			slist_.reset(current);
			throw std::runtime_error("SlistAppendError(append)");
		}
	}

	[[nodiscard]]
	curl_slist *getRaw() const noexcept
	{
		return slist_.get();
	}

private:
	std::unique_ptr<curl_slist, CurlSlistDeleter> slist_{nullptr};
};

} // namespace KaitoTokyo::CurlHelper
