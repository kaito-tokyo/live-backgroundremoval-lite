/*
 * Live Background Removal Lite - Global Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "GlobalContext.hpp"

#include <cstddef>
#include <regex>
#include <string_view>
#include <utility>

#include <curl/curl.h>
#include <fmt/format.h>

#include <CurlVectorWriter.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

namespace {

struct ResumeOnJThread {
	jthread_ns::jthread &threadStorage;

	bool await_ready() { return false; }

	void await_suspend(std::coroutine_handle<> h)
	{
		threadStorage = jthread_ns::jthread([h] { h.resume(); });
	}

	void await_resume() {}
};

} // namespace

GlobalContext::GlobalContext(const char *pluginName, const char *pluginVersion,
			     std::shared_ptr<const Logger::ILogger> logger, const char *latestVersionUrl)
	: pluginName_(pluginName),
	  pluginVersion_(pluginVersion),
	  logger_(std::move(logger)),
	  latestVersionUrl_(latestVersionUrl),
	  fetchLatestVersionTask_(fetchLatestVersion(std::allocator_arg, fetchLatestVersionTaskStorage_, this))
{
	fetchLatestVersionTask_.start();
}

std::string GlobalContext::getLatestVersion() const
{
	std::lock_guard lock(mutex_);
	return latestVersion_;
}

Async::Task<void> GlobalContext::fetchLatestVersion(std::allocator_arg_t, TaskStorage &, GlobalContext *self)
{
	std::shared_ptr<const Logger::ILogger> logger = self->logger_;

	co_await ResumeOnJThread{self->fetchLatestVersionThread_};

	const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
	if (!curl) {
		logger->error("Failed to initialize CURL for fetching latest version");
		throw std::runtime_error("InitError(GlobalContext::fetchLatestVersion)");
	}

	CurlHelper::CurlVectorWriterBuffer readBuffer;

	curl_easy_setopt(curl.get(), CURLOPT_URL, self->latestVersionUrl_.c_str());
	curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlVectorWriter);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl.get(), CURLOPT_MAXFILESIZE, 100L);
	curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

	const CURLcode res = curl_easy_perform(curl.get());

	if (res != CURLE_OK) {
		logger->error("Failed to fetch latest version from {}: {}", self->latestVersionUrl_,
			      curl_easy_strerror(res));
		throw std::runtime_error(
			fmt::format("NetworkError(GlobalContext::fetchLatestVersion):{}", curl_easy_strerror(res)));
	}

	std::size_t len = std::min(readBuffer.size(), std::size_t(100));
	std::string_view rawInput(readBuffer.data(), len);

	static const std::regex extractPattern(
		R"(^\s*(v?(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)(?:-(?:(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+(?:[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?)\s*$)");

	std::match_results<std::string_view::const_iterator> matches;

	if (!std::regex_match(rawInput.begin(), rawInput.end(), matches, extractPattern)) {
		logger->error("Invalid version format received: '{}'", rawInput);
		throw std::runtime_error("InvalidVersionFormat(GlobalContext::fetchLatestVersion)");
	}

	std::string_view extractedVersion(rawInput.data() + std::distance(rawInput.begin(), matches[1].first),
					  static_cast<std::size_t>(matches[1].length()));

	logger->info("Latest version on Internet is {}", extractedVersion);

	std::scoped_lock lock(self->mutex_);
	self->latestVersion_ = extractedVersion;
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
