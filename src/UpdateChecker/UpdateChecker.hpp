/*
Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <future>
#include <stdexcept>
#include <string>

#include <curl/curl.h>

namespace KaitoTokyo {
namespace UpdateChecker {

inline std::future<std::string> fetchLatestVersion(const std::string &url)
{
	return std::async(std::launch::async, [url]() -> std::string {
		CURL *curl = curl_easy_init();
		if (!curl) {
			throw std::runtime_error("Failed to initialize curl");
		}
		std::string result;
		auto writeCallback = [](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
			size_t totalSize = size * nmemb;
			std::string *str = static_cast<std::string *>(userp);
			str->append(static_cast<char *>(contents), totalSize);
			return totalSize;
		};
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
				 static_cast<size_t (*)(void *, size_t, size_t, void *)>(writeCallback));
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (res != CURLE_OK) {
			throw std::runtime_error("curl_easy_perform() failed");
		}
		return result;
	});
}

} // namespace UpdateChecker
} // namespace KaitoTokyo
