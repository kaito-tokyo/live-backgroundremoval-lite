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

inline size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t totalSize = size * nmemb;
	std::string *str = static_cast<std::string *>(userp);
	str->append(static_cast<char *>(contents), totalSize);
	return totalSize;
}

inline std::string fetchLatestVersion(const std::string &url)
{
	if (url.empty()) {
		throw std::invalid_argument("URL must not be empty");
	}
	CURL *curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("Failed to initialize curl");
	}
	std::string result;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res != CURLE_OK) {
		throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
	}
	return result;
}

} // namespace UpdateChecker
} // namespace KaitoTokyo
