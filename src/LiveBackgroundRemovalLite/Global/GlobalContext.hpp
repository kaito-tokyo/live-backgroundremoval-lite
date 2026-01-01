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

#pragma once

#ifdef __APPLE__
#include <jthread.hpp>
#endif

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <KaitoTokyo/CurlHelper/CurlHandle.hpp>
#include <KaitoTokyo/Logger/ILogger.hpp>

#include "PluginConfig.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

#ifdef __APPLE__
namespace jthread_ns = josuttis;
#else
namespace jthread_ns = std;
#endif

class GlobalContext : public std::enable_shared_from_this<GlobalContext> {
public:
	GlobalContext(std::shared_ptr<PluginConfig> pluginConfig, std::shared_ptr<const Logger::ILogger> logger,
		      std::string pluginName, std::string pluginVersion, std::string latestVersionUrl);

	~GlobalContext() noexcept;

	std::string getPluginName() const noexcept;
	std::string getPluginVersion() const noexcept;
	std::shared_ptr<const Logger::ILogger> getLogger() const noexcept;
	std::optional<std::string> getLatestVersion() const;

	void checkForUpdates();

private:
	const std::string pluginName_;
	const std::string pluginVersion_;
	const std::shared_ptr<const Logger::ILogger> logger_;
	const std::string latestVersionUrl_;
	const std::shared_ptr<PluginConfig> pluginConfig_;

	mutable std::mutex mutex_;
	CurlHelper::CurlHandle curl_;

	std::optional<std::string> latestVersion_{std::nullopt};
	jthread_ns::jthread fetchLatestVersionThread_;

	static void fetchLatestVersionThread(jthread_ns::stop_token stoken, std::shared_ptr<GlobalContext> self);
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
