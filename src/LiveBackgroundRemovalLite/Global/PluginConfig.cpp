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

#include "PluginConfig.hpp"

#include <filesystem>
#include <fstream>

#include <ObsUnique.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

PluginConfig PluginConfig::load(std::shared_ptr<const Logger::ILogger> logger)
{
	BridgeUtils::unique_bfree_char_t configPath = BridgeUtils::unique_obs_module_config_path("PluginConfig.json");

	BridgeUtils::unique_obs_data_t data(obs_data_create_from_json_file_safe(configPath.get(), ".bak"));

	PluginConfig pluginConfig;

	BridgeUtils::unique_bfree_char_t disableFlagPathRaw =
		BridgeUtils::unique_obs_module_config_path("PluginConfig_disableAutoCheckForUpdate.txt");
	if (disableFlagPathRaw) {
		pluginConfig.disableAutoCheckForUpdate = std::filesystem::exists(disableFlagPathRaw.get());
	}

	pluginConfig.selfieSegmenterParamPath =
		BridgeUtils::unique_obs_module_file("models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param")
			.get();

	pluginConfig.selfieSegmenterBinPath =
		BridgeUtils::unique_obs_module_file("models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin")
			.get();

	if (!data) {
		logger->info("No config file found, using default configuration");
		return pluginConfig;
	}

	if (const char *str = obs_data_get_string(data.get(), "selfieSegmenterParamPath")) {
		logger->info("Loaded selfieSegmenterParamPath from config: {}", str);
		pluginConfig.selfieSegmenterParamPath = str;
	}

	if (const char *str = obs_data_get_string(data.get(), "selfieSegmenterBinPath")) {
		logger->info("Loaded selfieSegmenterBinPath from config: {}", str);
		pluginConfig.selfieSegmenterBinPath = str;
	}

	return pluginConfig;
}

void PluginConfig::setDisableAutoCheckForUpdate(bool disabled)
{
	BridgeUtils::unique_bfree_char_t configPathRaw =
		BridgeUtils::unique_obs_module_config_path("PluginConfig.json");
	const std::filesystem::path path(configPathRaw.get());
	if (disabled) {
		if (!std::filesystem::exists(path)) {
			std::ofstream ofs(path);
		}
	} else {
		std::filesystem::remove(path);
	}
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
