/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Background Removal Lite - MainFilter Module
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

#include "MainFilterInfo.hpp"

#include <exception>
#include <memory>
#include <utility>

#include <KaitoTokyo/Logger/NullLogger.hpp>
#include <KaitoTokyo/ObsBridgeUtils/GsUnique.hpp>

#include "MainFilterContext.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

std::shared_ptr<Global::PluginConfig> g_pluginConfig_ = nullptr;
std::shared_ptr<Global::GlobalContext> g_globalContext_ = nullptr;

obs_source_info g_mainFilterInfo_ = {.id = "live_backgroundremoval_lite",
				     .type = OBS_SOURCE_TYPE_FILTER,
				     .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
				     .get_name = MainFilter::getName,
				     .create = MainFilter::create,
				     .destroy = MainFilter::destroy,
				     .get_width = MainFilter::getWidth,
				     .get_height = MainFilter::getHeight,
				     .get_defaults = MainFilter::getDefaults,
				     .get_properties = MainFilter::getProperties,
				     .update = MainFilter::update,
				     .activate = MainFilter::activate,
				     .deactivate = MainFilter::deactivate,
				     .show = MainFilter::show,
				     .hide = MainFilter::hide,
				     .video_tick = MainFilter::videoTick,
				     .video_render = MainFilter::videoRender};

bool loadModule(std::shared_ptr<Global::PluginConfig> pluginConfig,
		std::shared_ptr<Global::GlobalContext> globalContext) noexcept
{
	g_pluginConfig_ = std::move(pluginConfig);
	g_globalContext_ = std::move(globalContext);
	obs_register_source(&g_mainFilterInfo_);
	return true;
}

void unloadModule() noexcept
{
	g_globalContext_.reset();
	g_pluginConfig_.reset();
}

const char *getName(void *) noexcept
{
	return obs_module_text("pluginName");
}

void *create(obs_data_t *settings, obs_source_t *source) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	try {
		ObsBridgeUtils::GraphicsContextGuard graphicsContextGuard;
		auto self = std::make_shared<MainFilterContext>(settings, source, g_pluginConfig_, g_globalContext_);
		return new std::shared_ptr<MainFilterContext>(self);
	} catch (const std::exception &e) {
		logger->error("CreateMainFilterContextExceptionError", {{"message", e.what()}});
		return nullptr;
	} catch (...) {
		logger->error("CreateMainFilterContextUnknownExceptionError");
		return nullptr;
	}
}

void destroy(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	(*selfPtr)->shutdown();
	delete selfPtr;

	ObsBridgeUtils::GraphicsContextGuard graphicsContextGuard;
	ObsBridgeUtils::GsUnique::drain();
}

std::uint32_t getWidth(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return 0;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return 0;
	}

	return (*selfPtr)->getWidth();
}

std::uint32_t getHeight(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return 0;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return 0;
	}

	return (*selfPtr)->getHeight();
}

void getDefaults(obs_data_t *data) noexcept
{
	MainFilterContext::getDefaults(data);
}

obs_properties_t *getProperties(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return obs_properties_create();
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return obs_properties_create();
	}

	try {
		return (*selfPtr)->getProperties();
	} catch (const std::exception &e) {
		logger->error("GetPropertiesExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("GetPropertiesUnknownExceptionError");
	}

	return obs_properties_create();
}

void update(void *data, obs_data_t *settings) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	try {
		(*selfPtr)->update(settings);
	} catch (const std::exception &e) {
		logger->error("UpdateExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("UpdateUnknownExceptionError");
	}
}

void activate(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	try {
		(*selfPtr)->activate();
	} catch (const std::exception &e) {
		logger->error("ActivateExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("ActivateUnknownExceptionError");
	}
}

void deactivate(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	try {
		(*selfPtr)->deactivate();
	} catch (const std::exception &e) {
		logger->error("DeactivateExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("DeactivateUnknownExceptionError");
	}
}

void show(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	try {
		(*selfPtr)->show();
	} catch (const std::exception &e) {
		logger->error("ShowExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("ShowUnknownExceptionError");
	}
}

void hide(void *data) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	try {
		(*selfPtr)->hide();
	} catch (const std::exception &e) {
		logger->error("HideExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("HideUnknownExceptionError");
	}
}

void videoTick(void *data, float seconds) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	try {
		(*selfPtr)->videoTick(seconds);
	} catch (const std::exception &e) {
		logger->error("VideoTickExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("VideoTickUnknownExceptionError");
	}
}

void videoRender(void *data, gs_effect_t *) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("FilterDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	if (!*selfPtr) {
		logger->error("FilterSelfIsNullError");
		return;
	}

	try {
		(*selfPtr)->videoRender();
		ObsBridgeUtils::GsUnique::drain();
	} catch (const std::exception &e) {
		logger->error("VideoRenderExceptionError", {{"message", e.what()}});
	} catch (...) {
		logger->error("VideoRenderUnknownExceptionError");
	}
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
