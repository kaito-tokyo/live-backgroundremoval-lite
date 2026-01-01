/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Background Removal Lite - Global Module
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

#include "PluginConfig.hpp"

#include <filesystem>
#include <fstream>

#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

namespace {

constexpr auto kFirstRunOccurredFileName = "live-backgroundremoval-lite_PluginConfig_HasFirstRunOccurred.txt";
constexpr auto kAutoCheckForUpdateDisabledFileName =
	"live-backgroundremoval-lite_PluginConfig_AutoCheckForUpdateDisabled.txt";
constexpr auto kMediaPipeLandscapeSelfieSegmenterParamPath =
	"models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param";
constexpr auto kMediaPipeLandscapeSelfieSegmenterBinPath =
	"models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin";

std::filesystem::path obsToPath(const char *obsPath)
{
	return std::filesystem::path(reinterpret_cast<const char8_t *>(obsPath));
}

} // anonymous namespace

PluginConfig::~PluginConfig() noexcept = default;

bool PluginConfig::isFirstRun()
try {
	ObsBridgeUtils::unique_bfree_char_t configPathRaw(obs_module_config_path(kFirstRunOccurredFileName));
	if (!configPathRaw)
		return false;

	const std::filesystem::path path(obsToPath(configPathRaw.get()));

	if (std::filesystem::exists(path))
		return false;

	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	if (ec)
		return false;

	std::ofstream ofs(path);
	bool is_open = ofs.is_open();
	ofs.close();

	return is_open;
} catch (...) {
	return false;
}

void PluginConfig::setAutoCheckForUpdateEnabled()
{
	ObsBridgeUtils::unique_bfree_char_t configPathRaw(obs_module_config_path(kAutoCheckForUpdateDisabledFileName));

	if (!configPathRaw) {
		logger_->error("ModuleConfigPathError", {{"configFile", kAutoCheckForUpdateDisabledFileName}});
		throw std::runtime_error("ModuleConfigPathError(PluginConfig::setAutoCheckForUpdateEnabled)");
	}

	const std::filesystem::path path(obsToPath(configPathRaw.get()));

	std::filesystem::remove(path);

	disableAutoCheckForUpdate_ = false;
}

void PluginConfig::setAutoCheckForUpdateDisabled()
{
	ObsBridgeUtils::unique_bfree_char_t configPathRaw(obs_module_config_path(kAutoCheckForUpdateDisabledFileName));

	if (!configPathRaw) {
		logger_->error("ModuleConfigPathError", {{"configFile", kAutoCheckForUpdateDisabledFileName}});
		throw std::runtime_error("ModuleConfigPathError(PluginConfig::setAutoCheckForUpdateDisabled)");
	}

	const std::filesystem::path path(obsToPath(configPathRaw.get()));

	if (!std::filesystem::exists(path)) {
		std::filesystem::create_directories(path.parent_path());
		std::ofstream ofs(path);
	}

	disableAutoCheckForUpdate_ = true;
}

bool PluginConfig::isAutoCheckForUpdateEnabled() const noexcept
{
	return !disableAutoCheckForUpdate_;
}

std::filesystem::path PluginConfig::getMediaPipeLandscapeSelfieSegmenterParamPath() const noexcept
{
	return mediaPipeLandscapeSelfieSegmenterParamPath_;
}

std::filesystem::path PluginConfig::getMediaPipeLandscapeSelfieSegmenterBinPath() const noexcept
{
	return mediaPipeLandscapeSelfieSegmenterBinPath_;
}

PluginConfig::PluginConfig(std::shared_ptr<const Logger::ILogger> logger)
	: logger_(logger ? std::move(logger)
			 : throw std::invalid_argument("LoggerIsNullError(PluginConfig::PluginConfig)"))
{
}

std::unique_ptr<PluginConfig> PluginConfig::load(std::shared_ptr<const Logger::ILogger> logger)
{
	auto pluginConfig(std::unique_ptr<PluginConfig>(new PluginConfig(std::move(logger))));

	// --- HasFirstRunOccurred ---
	ObsBridgeUtils::unique_bfree_char_t hasFirstRunOccurredPathRaw(
		obs_module_config_path(kFirstRunOccurredFileName));

	if (!hasFirstRunOccurredPathRaw) {
		logger->error("ModuleConfigPathError", {{"configFile", kFirstRunOccurredFileName}});
		throw std::runtime_error("ModuleConfigPathError(PluginConfig::load)");
	}

	std::filesystem::path hasFirstRunOccurredPath(obsToPath(hasFirstRunOccurredPathRaw.get()));

	pluginConfig->hasFirstRunOccurred_ = std::filesystem::exists(hasFirstRunOccurredPath);

	// --- AutoCheckForUpdateDisabled ---
	ObsBridgeUtils::unique_bfree_char_t disableFlagPathRaw(
		obs_module_config_path(kAutoCheckForUpdateDisabledFileName));

	if (!disableFlagPathRaw) {
		logger->error("ModuleConfigPathError", {{"configFile", kAutoCheckForUpdateDisabledFileName}});
		throw std::runtime_error("ModuleConfigPathError(PluginConfig::load)");
	}

	std::filesystem::path disableFlagPath(obsToPath(disableFlagPathRaw.get()));

	pluginConfig->disableAutoCheckForUpdate_ = std::filesystem::exists(disableFlagPath);

	// --- mediaPipeLandscapeSelfieSegmenterParamPath ---
	ObsBridgeUtils::unique_bfree_char_t mediaPipeLandscapeSelfieSegmenterParamPathRaw(
		obs_module_file(kMediaPipeLandscapeSelfieSegmenterParamPath));

	if (!mediaPipeLandscapeSelfieSegmenterParamPathRaw) {
		logger->error("ModuleFileError", {{"file", kMediaPipeLandscapeSelfieSegmenterParamPath}});
		throw std::runtime_error("ModuleFileError(PluginConfig::load)");
	}

	std::filesystem::path mediaPipeLandscapeSelfieSegmenterParamPath(
		obsToPath(mediaPipeLandscapeSelfieSegmenterParamPathRaw.get()));

	pluginConfig->mediaPipeLandscapeSelfieSegmenterParamPath_ = mediaPipeLandscapeSelfieSegmenterParamPath;

	// --- mediaPipeLandscapeSelfieSegmenterBinPath ---
	ObsBridgeUtils::unique_bfree_char_t mediaPipeLandscapeSelfieSegmenterBinPathRaw(
		obs_module_file(kMediaPipeLandscapeSelfieSegmenterBinPath));

	if (!mediaPipeLandscapeSelfieSegmenterBinPathRaw) {
		logger->error("ModuleFileError", {{"file", kMediaPipeLandscapeSelfieSegmenterBinPath}});
		throw std::runtime_error("ModuleFileError(PluginConfig::load)");
	}

	std::filesystem::path mediaPipeLandscapeSelfieSegmenterBinPath(
		obsToPath(mediaPipeLandscapeSelfieSegmenterBinPathRaw.get()));

	pluginConfig->mediaPipeLandscapeSelfieSegmenterBinPath_ = mediaPipeLandscapeSelfieSegmenterBinPath;

	return pluginConfig;
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
