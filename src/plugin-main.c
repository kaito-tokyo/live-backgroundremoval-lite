/*
OBS Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

#include "MainPluginContext.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

struct obs_source_info main_plugin_context = {.id = "backgroundremoval_lite",
					      .type = OBS_SOURCE_TYPE_FILTER,
					      .output_flags = OBS_SOURCE_VIDEO,
					      .get_name = main_plugin_context_get_name,
					      .create = main_plugin_context_create,
					      .destroy = main_plugin_context_destroy,
					      .get_width = main_plugin_context_get_width,
					      .get_height = main_plugin_context_get_height,
					      .get_defaults = main_plugin_context_get_defaults,
					      .get_properties = main_plugin_context_get_properties,
					      .update = main_plugin_context_update,
					      .activate = main_plugin_context_activate,
					      .deactivate = main_plugin_context_deactivate,
					      .show = main_plugin_context_show,
					      .hide = main_plugin_context_hide,
					      .video_tick = main_plugin_context_video_tick,
					      .video_render = main_plugin_context_video_render,
					      .filter_video = main_plugin_context_filter_video};

bool obs_module_load(void)
{
	obs_register_source(&main_plugin_context);
	if (main_plugin_context_module_load()) {
		obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
		return true;
	} else {
		obs_log(LOG_ERROR, "Failed to load plugin");
		return false;
	}
}

void obs_module_unload(void)
{
	main_plugin_context_module_unload();
	obs_log(LOG_INFO, "plugin unloaded");
}
