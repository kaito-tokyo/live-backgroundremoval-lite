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

	std::string mediapipeLandscapeSelfieSegmenterParamPath =
		BridgeUtils::unique_obs_module_file("models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param")
			.get();

	std::string mediapipeLandscapeSelfieSegmenterBinPath =
		BridgeUtils::unique_obs_module_file("models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin")
			.get();

	std::string ppHumansegv2LiteSelfieSegmenterParamPath =
		BridgeUtils::unique_obs_module_file("models/portrait_pp_humansegv2_lite_int8.ncnn.param")
			.get();

	std::string ppHumansegv2LiteSelfieSegmenterBinPath =
		BridgeUtils::unique_obs_module_file("models/portrait_pp_humansegv2_lite_int8.ncnn.bin")
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
			return pluginConfig;
		}

		if (const char *str = obs_data_get_string(data.get(), "latestVersionURL")) {
			pluginConfig.latestVersionURL = str;
		}

		if (const char *str = obs_data_get_string(data.get(), "mediapipeLandscapeSelfieSegmenterBinPath")) {
			pluginConfig.mediapipeLandscapeSelfieSegmenterBinPath = str;
		}

		if (const char *str = obs_data_get_string(data.get(), "mediapipeLandscapeSelfieSegmenterParamPath")) {
			pluginConfig.mediapipeLandscapeSelfieSegmenterParamPath = str;
		}

		if (const char *str = obs_data_get_string(data.get(), "ppHumansegv2LiteSelfieSegmenterBinPath")) {
			pluginConfig.ppHumansegv2LiteSelfieSegmenterBinPath = str;
		}

		if (const char *str = obs_data_get_string(data.get(), "ppHumansegv2LiteSelfieSegmenterParamPath")) {
			pluginConfig.ppHumansegv2LiteSelfieSegmenterParamPath = str;
		}

		return pluginConfig;
	}
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
