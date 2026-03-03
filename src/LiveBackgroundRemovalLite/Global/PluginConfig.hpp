// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <memory>

#include <KaitoTokyo/Logger/ILogger.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

class PluginConfig {
public:
	~PluginConfig() noexcept;

	PluginConfig(const PluginConfig &) = delete;
	PluginConfig &operator=(const PluginConfig &) = delete;
	PluginConfig(PluginConfig &&);
	PluginConfig &operator=(PluginConfig &&);

	bool isFirstRun();
	void setAutoCheckForUpdateEnabled();
	void setAutoCheckForUpdateDisabled();
	bool isAutoCheckForUpdateEnabled() const noexcept;

	static std::unique_ptr<PluginConfig> load(std::shared_ptr<const Logger::ILogger> logger);
	static std::unique_ptr<PluginConfig> fallback(std::shared_ptr<const Logger::ILogger> logger);

private:
	PluginConfig(std::shared_ptr<const Logger::ILogger> logger);

	const std::shared_ptr<const Logger::ILogger> logger_;

	bool hasFirstRunOccurred_ = true;
	bool disableAutoCheckForUpdate_ = false;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
