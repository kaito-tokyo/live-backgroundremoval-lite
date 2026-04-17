// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>

#include <QCoreApplication>
#include <QTranslator>

#include <curl/curl.h>

#include <obs-module.h>

#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/Logger/NullLogger.hpp>
#include <KaitoTokyo/ObsBridgeUtils/ObsLogger.hpp>

#include <GlobalContext.hpp>
#include <MainFilterInfo.hpp>
#include <PluginConfig.hpp>
#include <StartupController.hpp>

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveBackgroundRemovalLite;

#define PLUGIN_NAME "live-backgroundremoval-lite"

#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#define QT_RESOURCE_PREFIX "/" PLUGIN_NAME "/" PLUGIN_VERSION

namespace {

const char latestVersionUrl[] = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/metadata/latest-version.txt";

std::shared_ptr<Global::PluginConfig> g_pluginConfig_;
std::shared_ptr<Global::GlobalContext> g_globalContext_;
std::shared_ptr<StartupUI::StartupController> g_startupController_;
std::unique_ptr<QTranslator> g_appTranslator_;

} // anonymous namespace

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

void handleFrontendEvent(enum obs_frontend_event event, void *)
try {
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		if (g_pluginConfig_ && g_startupController_ && g_pluginConfig_->isFirstRun()) {
			g_startupController_->showFirstRunDialog();
		}
	}
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[%s] Exception in frontend event handler: %s", PLUGIN_NAME, e.what());
} catch (...) {
	blog(LOG_ERROR, "[%s] Unknown exception in frontend event handler", PLUGIN_NAME);
}

bool obs_module_load(void)
try {
	Q_INIT_RESOURCE(resources);
	curl_global_init(CURL_GLOBAL_DEFAULT);

	const std::shared_ptr<const Logger::ILogger> logger =
		std::make_shared<ObsBridgeUtils::ObsLogger>("[" PLUGIN_NAME "]");

	const char *obsLocale = obs_get_locale();
	QString localeStr = QString::fromUtf8(obsLocale ? obsLocale : "en-US");
	localeStr.replace('-', '_');

	g_appTranslator_ = std::make_unique<QTranslator>();

	QString qmPath = QString(":%1/resources/%2.qm").arg(QT_RESOURCE_PREFIX).arg(localeStr);

	if (g_appTranslator_->load(qmPath)) {
		QCoreApplication::installTranslator(g_appTranslator_.get());
		logger->info("TranslationLoaded", {{"locale", localeStr.toStdString()}});
	} else {
		logger->info("DefaultTranslationLoaded");
	}

	std::shared_ptr<Global::PluginConfig> pluginConfig;
	try {
		pluginConfig = Global::PluginConfig::load(logger);
	} catch (const std::exception &e) {
		logger->error("PluginConfigLoadError", {{"error", e.what()}});
		pluginConfig = Global::PluginConfig::fallback(logger);
	} catch (...) {
		logger->error("PluginConfigLoadUnknownError");
		pluginConfig = Global::PluginConfig::fallback(logger);
	}

	g_pluginConfig_ = std::move(pluginConfig);
	g_globalContext_ = std::make_shared<Global::GlobalContext>(g_pluginConfig_, logger, PLUGIN_NAME, PLUGIN_VERSION,
								   latestVersionUrl);

	g_globalContext_->checkForUpdates();

	g_startupController_ = std::make_shared<StartupUI::StartupController>(g_pluginConfig_, g_globalContext_);

	if (!MainFilter::loadModule(g_pluginConfig_, g_globalContext_)) {
		throw std::runtime_error("MainFilterLoadModuleError(obs_module_load)");
	}

	obs_frontend_add_event_callback(handleFrontendEvent, nullptr);

	blog(LOG_INFO, "[%s] plugin loaded successfully (version %s)", PLUGIN_NAME, PLUGIN_VERSION);
	return true;
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[%s] %s", PLUGIN_NAME, e.what());
	blog(LOG_ERROR, "[%s] plugin load failed (version %s): %s", PLUGIN_NAME, PLUGIN_VERSION, e.what());
	return false;
} catch (...) {
	blog(LOG_ERROR, "[%s] plugin load failed (version %s)", PLUGIN_NAME, PLUGIN_VERSION);
	return false;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(handleFrontendEvent, nullptr);
	MainFilter::unloadModule();
	g_startupController_.reset();
	g_globalContext_.reset();
	g_pluginConfig_.reset();
	if (!g_appTranslator_->isEmpty()) {
		QCoreApplication::removeTranslator(g_appTranslator_.get());
	}
	g_appTranslator_.reset();
	curl_global_cleanup();
	Q_CLEANUP_RESOURCE(resources);
	blog(LOG_INFO, "[%s] plugin unloaded", PLUGIN_NAME);
}
