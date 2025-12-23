/*
 * Live Background Removal Lite - MainFilter Module
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

#include "MainPluginContext.h"

#include <exception>
#include <memory>

#include <obs-module.h>

#include <GsUnique.hpp>
#include <NullLogger.hpp>
#include <ILogger.hpp>

#include <UpdateChecker.hpp>

#include "PluginConfig.hpp"

#include "../plugin-support.h"

using namespace KaitoTokyo::Logger;
using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

std::string g_pluginName;
std::string g_pluginVersion;
std::shared_ptr<const Logger::ILogger> g_logger_ = nullptr;

inline bool loadModule(std::string pluginName, std::string pluginVersion,
		       std::shared_ptr<const Logger::ILogger> logger) noexcept
{
	g_pluginName = std::move(pluginName);
	g_pluginVersion = std::move(pluginVersion);
	g_logger_ = std::move(logger);
	return true;
}

inline void unloadModule() noexcept
{
	g_logger_.reset();
}

inline const char *getName(void *)
{
	return obs_module_text("pluginName");
}

void *create(obs_data_t *settings, obs_source_t *source)
{
	const auto logger = g_logger_;
	GraphicsContextGuard graphicsContextGuard;
	try {
		auto self = std::make_shared<MainPluginContext>(settings, source, latestVersionFuture, logger);
		return new std::shared_ptr<MainPluginContext>(self);
	} catch (const std::exception &e) {
		logger->logException(e, "Failed to create main plugin context");
		return nullptr;
	} catch (...) {
		logger->error("Failed to create main plugin context: unknown error");
		return nullptr;
	}
}

void main_plugin_context_destroy(void *data)
{
	const ILogger &logger = loggerInstance();

	if (!data) {
		logger.error("main_plugin_context_destroy called with null data");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	if (!selfPtr) {
		logger.error("main_plugin_context_destroy called with null MainPluginContext pointer");
		return;
	}

	auto self = selfPtr->get();
	if (!self) {
		logger.error("main_plugin_context_destroy called with null MainPluginContext");
		return;
	}

	self->shutdown();
	delete selfPtr;

	GraphicsContextGuard graphicsContextGuard;
	GsUnique::drain();
}

std::uint32_t main_plugin_context_get_width(void *data)
{
	const ILogger &logger = loggerInstance();

	if (!data) {
		logger.error("main_plugin_context_get_width called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data)->get();
	if (!self) {
		logger.error("main_plugin_context_get_width called with null MainPluginContext");
		return 0;
	}

	return self->getWidth();
}

std::uint32_t main_plugin_context_get_height(void *data)
{
	const ILogger &logger = loggerInstance();

	if (!data) {
		logger.error("main_plugin_context_get_height called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data)->get();
	if (!self) {
		logger.error("main_plugin_context_get_height called with null MainPluginContext");
		return 0;
	}

	return self->getHeight();
}

void main_plugin_context_get_defaults(obs_data_t *data)
{
	MainPluginContext::getDefaults(data);
}

obs_properties_t *main_plugin_context_get_properties(void *data)
{
	const ILogger &logger = loggerInstance();

	if (!data) {
		logger.error("main_plugin_context_get_properties called with null data");
		return obs_properties_create();
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data)->get();
	if (!self) {
		logger.error("main_plugin_context_get_properties called with null MainPluginContext");
		return obs_properties_create();
	}

	try {
		return self->getProperties();
	} catch (const std::exception &e) {
		logger.logException(e, "Failed to get properties");
	} catch (...) {
		logger.error("Failed to get properties: unknown error");
	}

	return obs_properties_create();
}

void main_plugin_context_update(void *data, obs_data_t *settings)
{
	const ILogger &logger = loggerInstance();

	if (!data) {
		logger.error("main_plugin_context_update called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data)->get();
	if (!self) {
		logger.error("main_plugin_context_update called with null MainPluginContext");
		return;
	}

	try {
		self->update(settings);
	} catch (const std::exception &e) {
		logger.logException(e, "Failed to update main plugin context");
	} catch (...) {
		logger.error("Failed to update main plugin context: unknown error");
	}
}

void main_plugin_context_video_tick(void *data, float seconds)
{
	const ILogger &logger = loggerInstance();

	if (!data) {
		logger.error("main_plugin_context_video_tick called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data)->get();
	if (!self) {
		logger.error("main_plugin_context_video_tick called with null MainPluginContext");
		return;
	}

	try {
		self->videoTick(seconds);
	} catch (const std::exception &e) {
		logger.logException(e, "Failed to tick main plugin context");
	} catch (...) {
		logger.error("Failed to tick main plugin context: unknown error");
	}
}

void main_plugin_context_video_render(void *data, gs_effect_t *)
{
	const ILogger &logger = loggerInstance();

	if (!data) {
		logger.error("main_plugin_context_video_render called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data)->get();
	if (!self) {
		logger.error("main_plugin_context_video_render called with null MainPluginContext");
		return;
	}

	try {
		self->videoRender();
		GsUnique::drain();
	} catch (const std::exception &e) {
		logger.logException(e, "Failed to render video in main plugin context");
	} catch (...) {
		logger.error("Failed to render video in main plugin context: unknown error");
	}
}

struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame)
{
	const ILogger &logger = loggerInstance();

	if (!frame) {
		logger.error("main_plugin_context_filter_video called with null frame");
		return nullptr;
	}

	if (!data) {
		logger.error("main_plugin_context_filter_video called with null data");
		return frame;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data)->get();
	if (!self) {
		logger.error("main_plugin_context_filter_video called with null MainPluginContext");
		return frame;
	}

	try {
		return self->filterVideo(frame);
	} catch (const std::exception &e) {
		logger.logException(e, "Failed to filter video in main plugin context");
	} catch (...) {
		logger.error("Failed to filter video in main plugin context: unknown error");
	}

	return frame;
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
