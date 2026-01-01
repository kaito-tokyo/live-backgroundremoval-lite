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

#include <regex>
#include <string_view>
#include <utility>

#include <curl/curl.h>

#include <KaitoTokyo/CurlHelper/CurlWriteCallback.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

GlobalContext::GlobalContext(std::shared_ptr<PluginConfig> pluginConfig, std::shared_ptr<const Logger::ILogger> logger,
			     std::string pluginName, std::string pluginVersion, std::string latestVersionUrl)
	: pluginName_(std::move(pluginName)),
	  pluginVersion_(std::move(pluginVersion)),
	  logger_(logger ? std::move(logger)
			 : throw std::invalid_argument("LoggerIsNullError(GlobalContext::GlobalContext)")),
	  latestVersionUrl_(std::move(latestVersionUrl)),
	  pluginConfig_(pluginConfig
				? std::move(pluginConfig)
				: throw std::invalid_argument("PluginConfigIsNullError(GlobalContext::GlobalContext)"))
{
}

GlobalContext::~GlobalContext() noexcept
{
	if (fetchLatestVersionThread_.joinable()) {
		fetchLatestVersionThread_.request_stop();
		try {
			fetchLatestVersionThread_.join();
		} catch (...) {
			logger_->error("UnrecoverableError");
		}
	}
};

std::string GlobalContext::getPluginName() const noexcept
{
	return pluginName_;
}

std::string GlobalContext::getPluginVersion() const noexcept
{
	return pluginVersion_;
}

std::optional<std::string> GlobalContext::getLatestVersion() const
{
	std::lock_guard lock(mutex_);
	return latestVersion_;
}

void GlobalContext::checkForUpdates()
{
	if (pluginConfig_->isAutoCheckForUpdateEnabled()) {
		fetchLatestVersionThread_ =
			jthread_ns::jthread(&GlobalContext::fetchLatestVersionThread, shared_from_this());
	}
}

void GlobalContext::fetchLatestVersionThread(jthread_ns::stop_token stoken, std::shared_ptr<GlobalContext> self)
try {
	std::vector<char> readBuffer;

	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_URL, self->latestVersionUrl_.c_str());
	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_FOLLOWLOCATION, 2L);
	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_MAXFILESIZE, 100L);

	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_XFERINFODATA, &stoken);
	curl_easy_setopt(
		self->curl_.getRaw(), CURLOPT_XFERINFOFUNCTION,
		+[](void *clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int {
			auto *token = static_cast<jthread_ns::stop_token *>(clientp);
			return token->stop_requested() ? 1 : 0;
		});
	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_NOPROGRESS, 0L);

	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(self->curl_.getRaw(), CURLOPT_NOSIGNAL, 1L);

	const CURLcode res = curl_easy_perform(self->curl_.getRaw());

	if (res == CURLE_ABORTED_BY_CALLBACK) {
		self->logger_->warn("FetchLatestVersionCancelled");
		return;
	}

	if (res != CURLE_OK) {
		self->logger_->error("CurlPerformError", {{"message", curl_easy_strerror(res)}});
		return;
	}

	std::size_t len = std::min(readBuffer.size(), std::size_t(100));
	std::string_view rawInput(readBuffer.data(), len);

	static const std::regex extractPattern(
		R"(^\s*(v?(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)(?:-(?:(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+(?:[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?)\s*$)");

	std::match_results<std::string_view::const_iterator> matches;

	if (!std::regex_match(rawInput.begin(), rawInput.end(), matches, extractPattern)) {
		self->logger_->error("InvalidVersinoFormatError", {{"rawInput", rawInput}});
		return;
	}

	std::string_view latestVersion(rawInput.data() + std::distance(rawInput.begin(), matches[1].first),
				       static_cast<std::size_t>(matches[1].length()));

	self->logger_->info("LatestVersionObtained", {{"latestVersion", latestVersion}});

	std::scoped_lock lock(self->mutex_);
	self->latestVersion_ = latestVersion;
} catch (const std::exception &e) {
	self->logger_->error("GeneralError", {{"message", e.what()}});
} catch (...) {
	self->logger_->error("UnrecoverableError");
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
