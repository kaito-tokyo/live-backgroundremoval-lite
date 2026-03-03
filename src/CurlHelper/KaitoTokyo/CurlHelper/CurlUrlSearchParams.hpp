// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

class CurlUrlSearchParams {
public:
	explicit CurlUrlSearchParams(CURL *curl)
		: curl_(curl ? curl : throw std::invalid_argument("CurlIsNullError(CurlUrlSearchParams)"))
	{
	}

	~CurlUrlSearchParams() noexcept = default;

	CurlUrlSearchParams(const CurlUrlSearchParams &) = delete;
	CurlUrlSearchParams &operator=(const CurlUrlSearchParams &) = delete;
	CurlUrlSearchParams(CurlUrlSearchParams &&) = delete;
	CurlUrlSearchParams &operator=(CurlUrlSearchParams &&) = delete;

	void append(std::string name, std::string value) { params_.emplace_back(name, value); }

	[[nodiscard]]
	std::string toString() const
	{
		auto curlEasyEscape = [curl = curl_](const std::string &str) {
			return std::unique_ptr<char, decltype(&curl_free)>(
				curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length())), curl_free);
		};

		std::ostringstream oss;
		for (std::size_t i = 0; i < params_.size(); i++) {
			if (i > 0) {
				oss << "&";
			}

			const std::string &key = params_[i].first;
			const std::string &value = params_[i].second;

			auto escapedKey = curlEasyEscape(key);
			if (!escapedKey) {
				throw std::runtime_error("KeyEncodeError(toString)");
			}

			auto escapedValue = curlEasyEscape(value);
			if (!escapedValue) {
				throw std::runtime_error("ValueEncodeError(toString)");
			}

			oss << escapedKey.get() << "=" << escapedValue.get();
		}
		return oss.str();
	}

private:
	CURL *const curl_;
	std::vector<std::pair<std::string, std::string>> params_;
};

} // namespace KaitoTokyo::CurlHelper
