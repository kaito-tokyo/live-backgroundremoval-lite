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

#include "MainPluginContext.h"

#include <stdexcept>

#include "plugin-support.h"

#include <obs-module.h>

using namespace kaito_tokyo::obs_backgroundremoval_lite;

const char *main_plugin_context_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("pluginName");
}

void *main_plugin_context_create(obs_data_t *settings, obs_source_t *source)
try {
	auto self = std::make_shared<MainPluginContext>(settings, source);
	return new std::shared_ptr<MainPluginContext>(self);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to create main plugin context: %s", e.what());
	return nullptr;
} catch (...) {
	obs_log(LOG_ERROR, "Failed to create main plugin context: unknown error");
	return nullptr;
}

void main_plugin_context_destroy(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_destroy called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	delete self;
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to destroy main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to destroy main plugin context: unknown error");
}

uint32_t main_plugin_context_get_width(void *data)
{
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_get_width called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getWidth();
}

uint32_t main_plugin_context_get_height(void *data)
{
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_get_height called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getHeight();
}

void main_plugin_context_get_defaults(obs_data_t *data)
{
	MainPluginContext::getDefaults(data);
}

obs_properties_t *main_plugin_context_get_properties(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_get_properties called with null data");
		return nullptr;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getProperties();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to get properties: %s", e.what());
	return nullptr;
} catch (...) {
	obs_log(LOG_ERROR, "Failed to get properties: unknown error");
	return nullptr;
}

void main_plugin_context_update(void *data, obs_data_t *settings)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_update called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->update(settings);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to update main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to update main plugin context: unknown error");
}

void main_plugin_context_activate(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_activate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->activate();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to activate main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to activate main plugin context: unknown error");
}

void main_plugin_context_deactivate(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "showdraw_deactivate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->deactivate();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to deactivate main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to deactivate main plugin context: unknown error");
}

void main_plugin_context_show(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_show called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->show();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to show main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to show main plugin context: unknown error");
}

void main_plugin_context_hide(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_hide called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->hide();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to hide main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to hide main plugin context: unknown error");
}

void main_plugin_context_video_tick(void *data, float seconds)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_video_tick called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->videoTick(seconds);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to tick main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to tick main plugin context: unknown error");
}

void main_plugin_context_video_render(void *data, gs_effect_t *_unused)
try {
	UNUSED_PARAMETER(_unused);

	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_video_render called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
    self->get()->videoRender();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to render video in main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to render video in main plugin context: unknown error");
}

struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_filter_video called with null data");
		return frame;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->filterVideo(frame);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to filter video in main plugin context: %s", e.what());
	return frame;
} catch (...) {
	obs_log(LOG_ERROR, "Failed to filter video in main plugin context: unknown error");
	return frame;
}

bool main_plugin_context_load() try {
	return true;
} catch (const std::exception &e) {
    obs_log(LOG_ERROR, "Failed to load main plugin context: %s", e.what());
} catch (...) {
    obs_log(LOG_ERROR, "Failed to load main plugin context: unknown error");
}

void main_plugin_context_unload() try {
} catch (const std::exception &e) {
    obs_log(LOG_ERROR, "Failed to unload main plugin context: %s", e.what());
} catch (...) {
    obs_log(LOG_ERROR, "Failed to unload main plugin context: unknown error");
}

namespace kaito_tokyo {
namespace obs_showdraw {

MainPluginContext::MainPluginContext(obs_data_t *_settings, obs_source_t *_source)
	: settings{_settings},
	  source{_source}
{
	update(settings);
}

MainPluginContext::~MainPluginContext() noexcept
{
}

uint32_t MainPluginContext::getWidth() const noexcept
{
	return width;
}

uint32_t MainPluginContext::getHeight() const noexcept
{
	return height;
}

void MainPluginContext::getDefaults(obs_data_t *data)
{
	UNUSED_PARAMETER(data);
}

obs_properties_t *MainPluginContext::getProperties()
{
    return nullptr;
}

void MainPluginContext::update(obs_data_t *_settings)
{
    settings = _settings;
}

void MainPluginContext::activate()
{
}

void MainPluginContext::deactivate()
{
}

void MainPluginContext::show()
{
}

void MainPluginContext::hide()
{
}

void MainPluginContext::videoTick(float seconds)
{
    UNUSED_PARAMETER(seconds);
}

void MainPluginContext::videoRender()
{
	obs_source_skip_video_filter(source);
}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
{
    width = frame->width;
	height = frame->height;
	return frame;
}

} // namespace obs_showdraw
} // namespace kaito_tokyo
