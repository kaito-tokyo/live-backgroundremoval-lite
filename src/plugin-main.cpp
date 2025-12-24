/*
 * Live Background Removal Lite
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

#include <memory>

#include <QtResource>

#include <curl/curl.h>

#include <obs-module.h>

#include <GlobalContext.hpp>
#include <MainFilterInfo.hpp>
#include <ObsLogger.hpp>
#include <StartupController.hpp>

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveBackgroundRemovalLite;

#define PLUGIN_NAME "live-backgroundremoval-lite"

#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

const char latestVersionUrl[] = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/metadata/latest-version.txt";

std::shared_ptr<Global::GlobalContext> g_globalContext_;
std::shared_ptr<StartupUI::StartupController> g_startupController_;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

void handleFrontendEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		g_startupController_->showFirstRunDialog();
	}
}

bool obs_module_load(void)
{
	Q_INIT_RESOURCE(resources);

	curl_global_init(CURL_GLOBAL_DEFAULT);

	const std::shared_ptr<const Logger::ILogger> logger =
		std::make_shared<BridgeUtils::ObsLogger>("[" PLUGIN_NAME "]");

	g_globalContext_ =
		std::make_shared<Global::GlobalContext>(PLUGIN_NAME, PLUGIN_VERSION, logger, latestVersionUrl);

	g_startupController_ = std::make_shared<StartupUI::StartupController>(g_globalContext_);

	if (!MainFilter::loadModule(g_globalContext_)) {
		logger->error("failed to load plugin");
		return false;
	}

	obs_frontend_add_event_callback(handleFrontendEvent, nullptr);

	logger->info("plugin loaded successfully (version {})", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	const auto logger = g_globalContext_->logger_;
	MainFilter::unloadModule();
	g_globalContext_.reset();
	curl_global_cleanup();
	logger->info("plugin unloaded");
}
