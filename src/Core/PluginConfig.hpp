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

#include "../BridgeUtils/ObsUnique.hpp"

namespace KaitoTokyo {
namespace BackgroundRemovalLite {

struct PluginConfig {
    std::string latestVersionURL = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/metadata/latest-version.txt";

    static PluginConfig load() {
        using namespace KaitoTokyo::BridgeUtils;

        unique_bfree_char_t configPath(obs_module_config_path("PluginConfig.json"));
        unique_obs_data_t data(obs_data_create_from_json_file_safe(configPath.get(), ".bak"));

        PluginConfig pluginConfig;

        if (data) {
            pluginConfig.latestVersionURL = obs_data_get_string(data.get(), "latestVersionURL");
        }

        return pluginConfig;
    }
};

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
