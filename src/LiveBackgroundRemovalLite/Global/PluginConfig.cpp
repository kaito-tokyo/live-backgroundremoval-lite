// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginConfig.hpp"

#include <filesystem>
#include <fstream>

#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

namespace {

constexpr auto kFirstRunOccurredFileName = "live-backgroundremoval-lite_PluginConfig_HasFirstRunOccurred.txt";
constexpr auto kAutoCheckForUpdateDisabledFileName =
	"live-backgroundremoval-lite_PluginConfig_AutoCheckForUpdateDisabled.txt";

/**
 * @brief Converts an OBS path (C-style UTF-8 string) to a std::filesystem::path.
 */
std::filesystem::path obsToPath(const char *obsPath)
{
#ifdef _WIN32
	// Allow usage of deprecated u8path on Windows for brief implementation
	return std::filesystem::u8path(obsPath);
#else
	return std::filesystem::path(obsPath);
#endif
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

	return pluginConfig;
}

std::unique_ptr<PluginConfig> PluginConfig::fallback(std::shared_ptr<const Logger::ILogger> logger)
{
	return std::unique_ptr<PluginConfig>(new PluginConfig(logger));
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
