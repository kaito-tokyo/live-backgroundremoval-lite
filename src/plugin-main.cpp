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

#include <obs-module.h>

// #include <ObsLogger.hpp>

// #include <MainFilterInfo.hpp>

#include <SharedTask.hpp>

// using namespace KaitoTokyo;
// using namespace KaitoTokyo::LiveBackgroundRemovalLite;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

// obs_source_info main_plugin_context = {.id = "live_backgroundremoval_lite",
// 				       .type = OBS_SOURCE_TYPE_FILTER,
// 				       .output_flags = OBS_SOURCE_VIDEO,
// 				       .get_name = main_plugin_context_get_name,
// 				       .create = main_plugin_context_create,
// 				       .destroy = main_plugin_context_destroy,
// 				       .get_width = main_plugin_context_get_width,
// 				       .get_height = main_plugin_context_get_height,
// 				       .get_defaults = main_plugin_context_get_defaults,
// 				       .get_properties = main_plugin_context_get_properties,
// 				       .update = main_plugin_context_update,
// 				       .video_tick = main_plugin_context_video_tick,
// 				       .video_render = main_plugin_context_video_render,
// 				       .filter_video = main_plugin_context_filter_video};

#define PLUGIN_NAME "live-backgroundremoval-lite"

#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

// const obs_source_info g_mainFilterInfo_ = {
// 	.id = "live_backgroundremoval_lite",
// 	.type = OBS_SOURCE_TYPE_FILTER,
// 	.output_flags = OBS_SOURCE_VIDEO,
// 	.get_name = MainFilter::getName,
// };

bool obs_module_load(void)
{
	// const std::shared_ptr<const Logger::ILogger> logger =
	// 	std::make_shared<BridgeUtils::ObsLogger>("[" PLUGIN_NAME "] ");
	// if (MainFilter::loadModule(PLUGIN_NAME, PLUGIN_VERSION, logger)) {
	// 	obs_register_source(&g_mainFilterInfo_);
	// } else {
	// 	logger->error("failed to load plugin");
	// 	return false;
	// }

	// logger->info("plugin loaded successfully (version {})", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	// main_plugin_context_module_unload();
	// blog(LOG_INFO, "[" PLUGIN_NAME "] plugin unloaded");
}
