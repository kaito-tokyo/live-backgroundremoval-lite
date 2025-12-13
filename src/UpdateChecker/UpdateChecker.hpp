/*
 * UpdateChecker
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <limits>
#include <stdexcept>
#include <string>

#include <curl/curl.h>

namespace KaitoTokyo {
namespace UpdateChecker {

inline size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	// Check for multiplication overflow
	if (size != 0 && nmemb > (std::numeric_limits<size_t>::max() / size)) {
		return 0; // Signal error (prevents buffer overflow)
	}

	size_t totalSize = size * nmemb;
	try {
		std::string *str = static_cast<std::string *>(userp);
		str->append(static_cast<char *>(contents), totalSize);
	} catch (const std::bad_alloc &) {
		// Handle potential memory allocation failure
		return 0; // Signal error
	} catch (...) {
		// Handle other potential exceptions
		return 0; // Signal error
	}
	return totalSize;
}

/**
 * @brief Fetches content from a given URL synchronously (blocking).
 *
 * Performs an HTTP GET request to the specified URL using libcurl.
 * This function enforces SSL/TLS certificate verification, follows redirects
 * (up to 5), and times out after 10 seconds.
 *
 * @warning This is a synchronous, blocking call. Do not call it from a
 * sensitive thread (like a GUI main thread) as it may freeze the application.
 * Consider using `std::async` or a dedicated worker thread.
 *
 * @warning The caller MUST call `curl_global_init(CURL_GLOBAL_ALL)` once
 * at application startup (before any thread uses this function) and
 * `curl_global_cleanup()` once at application shutdown to ensure
 * thread-safety.
 *
 * @param url The URL to fetch content from. Must not be empty.
 * @return A `std::string` containing the body of the response.
 * @throw std::invalid_argument If the URL is empty.
 * @throw std::runtime_error If curl initialization fails or the transfer fails.
 */
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
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);       // 10 second timeout
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);      // Max 5 redirects

	// --- Security Settings ---
	// Explicitly set SSL/TLS certificate verification (default behavior for libcurl)
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	// Explicitly set hostname matching in the certificate (default behavior for libcurl)
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	// --- End Security Settings ---

	CURLcode res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
	}

	return result;
}

} // namespace UpdateChecker
} // namespace KaitoTokyo
