/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

#include <obs-module.h>

#include <ObsUnique.hpp>

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

struct PluginConfig {
	std::string latestVersionURL =
		"https://kaito-tokyo.github.io/live-backgroundremoval-lite/metadata/latest-version.txt";

	std::string selfieSegmenterParamPath =
		BridgeUtils::unique_obs_module_file("models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param")
			.get();

	std::string selfieSegmenterBinPath =
		BridgeUtils::unique_obs_module_file("models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin")
			.get();

	static PluginConfig load(const Logger::ILogger &logger)
	{
		using namespace KaitoTokyo::BridgeUtils;

		unique_bfree_char_t configPath;
		try {
			configPath = unique_obs_module_config_path("PluginConfig.json");
		} catch (const std::exception &e) {
			logger.warn("Failed to get config path: {}", e.what());
			return PluginConfig();
		}

		unique_obs_data_t data(obs_data_create_from_json_file_safe(configPath.get(), ".bak"));

		PluginConfig pluginConfig;

		if (!data) {
			logger.info("No config file found, using default configuration");
			return pluginConfig;
		}

		if (const char *str = obs_data_get_string(data.get(), "latestVersionURL")) {
			logger.info("Loaded latestVersionURL from config: {}", str);
			pluginConfig.latestVersionURL = str;
		}

		if (const char *str = obs_data_get_string(data.get(), "selfieSegmenterParamPath")) {
			logger.info("Loaded selfieSegmenterParamPath from config: {}", str);
			pluginConfig.selfieSegmenterParamPath = str;
		}

		if (const char *str = obs_data_get_string(data.get(), "selfieSegmenterBinPath")) {
			logger.info("Loaded selfieSegmenterBinPath from config: {}", str);
			pluginConfig.selfieSegmenterBinPath = str;
		}

		return pluginConfig;
	}
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
