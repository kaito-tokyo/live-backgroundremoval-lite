/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Background Removal Lite - AsyncSource Module
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

#include "AsyncSourceInfo.hpp"

#include <exception>
#include <memory>
#include <utility>

#include <KaitoTokyo/Logger/NullLogger.hpp>
#include <KaitoTokyo/ObsBridgeUtils/GsUnique.hpp>

#include "AsyncSourceContext.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::AsyncSource {

std::shared_ptr<Global::PluginConfig> g_pluginConfig_ = nullptr;
std::shared_ptr<Global::GlobalContext> g_globalContext_ = nullptr;

obs_source_info g_asyncSourceInfo_ = {
	.id = "live_backgroundremoval_lite_async_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
	.get_name = AsyncSource::getName,
	.create = AsyncSource::create,
	.destroy = AsyncSource::destroy,
	.get_width = AsyncSource::getWidth,
	.get_height = AsyncSource::getHeight,
	.get_defaults = AsyncSource::getDefaults,
	.get_properties = AsyncSource::getProperties,
	.update = AsyncSource::update,
	.video_tick = AsyncSource::videoTick,
};

bool loadModule(std::shared_ptr<Global::PluginConfig> pluginConfig,
		std::shared_ptr<Global::GlobalContext> globalContext) noexcept
{
	g_pluginConfig_ = std::move(pluginConfig);
	g_globalContext_ = std::move(globalContext);
	obs_register_source(&g_asyncSourceInfo_);
	return true;
}

void unloadModule() noexcept
{
	g_globalContext_.reset();
	g_pluginConfig_.reset();
}

const char *getName(void *) noexcept
{
	return obs_module_text("asyncSourceName");
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
		auto self = std::make_shared<AsyncSourceContext>(settings, source, g_pluginConfig_, g_globalContext_);
		return new std::shared_ptr<AsyncSourceContext>(self);
	} catch (const std::exception &e) {
		logger->error("CreateAsyncSourceContextExceptionError", {{"message", e.what()}});
		return nullptr;
	} catch (...) {
		logger->error("CreateAsyncSourceContextUnknownExceptionError");
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
		logger->error("SourceDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<AsyncSourceContext> *>(data);
	if (!*selfPtr) {
		logger->error("SourceSelfIsNullError");
		return;
	}

	(*selfPtr)->shutdown();
	delete selfPtr;

	ObsBridgeUtils::GraphicsContextGuard graphicsContextGuard;
	ObsBridgeUtils::GsUnique::drain();
}

uint32_t getWidth(void *data) noexcept
{
	if (!data) {
		return 0;
	}
	auto selfPtr = static_cast<std::shared_ptr<AsyncSourceContext> *>(data);
	if (!*selfPtr) {
		return 0;
	}
	return (*selfPtr)->getWidth();
}

uint32_t getHeight(void *data) noexcept
{
	if (!data) {
		return 0;
	}
	auto selfPtr = static_cast<std::shared_ptr<AsyncSourceContext> *>(data);
	if (!*selfPtr) {
		return 0;
	}
	return (*selfPtr)->getHeight();
}

void getDefaults(obs_data_t *data) noexcept
{
	AsyncSourceContext::getDefaults(data);
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
		logger->error("SourceDataIsNullError");
		return obs_properties_create();
	}

	auto selfPtr = static_cast<std::shared_ptr<AsyncSourceContext> *>(data);
	if (!*selfPtr) {
		logger->error("SourceSelfIsNullError");
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
		logger->error("SourceDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<AsyncSourceContext> *>(data);
	if (!*selfPtr) {
		logger->error("SourceSelfIsNullError");
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

void videoTick(void *data, float seconds) noexcept
{
	std::shared_ptr<const Logger::ILogger> logger;
	if (g_globalContext_ && g_globalContext_->getLogger()) {
		logger = g_globalContext_->getLogger();
	} else {
		logger = Logger::NullLogger::instance();
	}

	if (!data) {
		logger->error("SourceDataIsNullError");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<AsyncSourceContext> *>(data);
	if (!*selfPtr) {
		logger->error("SourceSelfIsNullError");
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

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::AsyncSource
