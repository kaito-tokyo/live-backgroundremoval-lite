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

#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef __APPLE__
#include <jthread.hpp>
#endif

#include <ILogger.hpp>
#include <PluginConfig.hpp>
#include <Task.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

#ifdef __APPLE__
namespace jthread_ns = josuttis;
#else
namespace jthread_ns = std;
#endif

class GlobalContext : public std::enable_shared_from_this<GlobalContext> {
public:
	GlobalContext(const char *pluginName, const char *pluginVersion, std::shared_ptr<const Logger::ILogger> logger,
		      const char *latestVersionUrl, std::shared_ptr<PluginConfig> pluginConfig);

	const std::string pluginName_;
	const std::string pluginVersion_;
	const std::shared_ptr<const Logger::ILogger> logger_;
	const std::string latestVersionUrl_;

	std::string getLatestVersion() const;

private:
	const std::shared_ptr<PluginConfig> pluginConfig_;

	std::optional<Async::Task<void>> fetchLatestVersionTask_ = std::nullopt;
	std::string latestVersion_;
	mutable std::mutex mutex_;

	jthread_ns::jthread fetchLatestVersionThread_;
	static Async::Task<void> fetchLatestVersion(std::shared_ptr<GlobalContext> self);
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
