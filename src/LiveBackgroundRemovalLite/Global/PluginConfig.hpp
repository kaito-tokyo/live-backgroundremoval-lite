/*
 * Live Background Removal Lite - Global Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

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
	std::filesystem::path getMediaPipeLandscapeSelfieSegmenterParamPath() const noexcept;
	std::filesystem::path getMediaPipeLandscapeSelfieSegmenterBinPath() const noexcept;

	static std::unique_ptr<PluginConfig> load(std::shared_ptr<const Logger::ILogger> logger);

private:
	PluginConfig(std::shared_ptr<const Logger::ILogger> logger);

	const std::shared_ptr<const Logger::ILogger> logger_;

	bool hasFirstRunOccurred_ = true;
	bool disableAutoCheckForUpdate_ = false;
	std::filesystem::path mediaPipeLandscapeSelfieSegmenterParamPath_;
	std::filesystem::path mediaPipeLandscapeSelfieSegmenterBinPath_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
