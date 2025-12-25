/*
 * Live Background Removal Lite - MainFilter Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#include <GsUnique.hpp>

#include "MainFilterContext.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

std::shared_ptr<Global::PluginConfig> g_pluginConfig_ = nullptr;
std::shared_ptr<Global::GlobalContext> g_globalContext_ = nullptr;

obs_source_info g_mainFilterInfo = {.id = "live_backgroundremoval_lite",
				    .type = OBS_SOURCE_TYPE_FILTER,
				    .output_flags = OBS_SOURCE_VIDEO,
				    .get_name = MainFilter::getName,
				    .create = MainFilter::create,
				    .destroy = MainFilter::destroy,
				    .get_width = MainFilter::getWidth,
				    .get_height = MainFilter::getHeight,
				    .get_defaults = MainFilter::getDefaults,
				    .get_properties = MainFilter::getProperties,
				    .update = MainFilter::update,
				    .video_tick = MainFilter::videoTick,
				    .video_render = MainFilter::videoRender,
				    .filter_video = MainFilter::filterVideo};

bool loadModule(std::shared_ptr<Global::PluginConfig> pluginConfig,
		std::shared_ptr<Global::GlobalContext> globalContext) noexcept
{
	g_pluginConfig_ = std::move(pluginConfig);
	g_globalContext_ = std::move(globalContext);
	obs_register_source(&g_mainFilterInfo);
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
	const auto logger = g_globalContext_->logger_;
	BridgeUtils::GraphicsContextGuard graphicsContextGuard;
	try {
		auto self = std::make_shared<MainFilterContext>(settings, source, g_pluginConfig_, g_globalContext_);
		return new std::shared_ptr<MainFilterContext>(self);
	} catch (const std::exception &e) {
		logger->logException(e, "Failed to create MainFilterContext");
		return nullptr;
	} catch (...) {
		logger->error("Failed to create MainFilterContext: unknown error");
		return nullptr;
	}
}

void destroy(void *data) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!data) {
		logger->error("MainFilter::destroy called with null data");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainFilterContext> *>(data);
	auto self = selfPtr->get();
	if (!self) {
		logger->error("MainFilter::destroy called with null MainFilterContext");
		return;
	}

	self->shutdown();
	delete selfPtr;

	BridgeUtils::GraphicsContextGuard graphicsContextGuard;
	BridgeUtils::GsUnique::drain();
}

std::uint32_t getWidth(void *data) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!data) {
		logger->error("MainFilter::getWidth called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainFilterContext> *>(data)->get();
	if (!self) {
		logger->error("MainFilter::getWidth called with null MainFilterContext");
		return 0;
	}

	return self->getWidth();
}

std::uint32_t getHeight(void *data) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!data) {
		logger->error("MainFilter::getHeight called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainFilterContext> *>(data)->get();
	if (!self) {
		logger->error("MainFilter::getHeight called with null MainFilterContext");
		return 0;
	}

	return self->getHeight();
}

void getDefaults(obs_data_t *data) noexcept
{
	MainFilterContext::getDefaults(data);
}

obs_properties_t *getProperties(void *data) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!data) {
		logger->error("MainFilter::getProperties called with null data");
		return obs_properties_create();
	}

	auto self = static_cast<std::shared_ptr<MainFilterContext> *>(data)->get();
	if (!self) {
		logger->error("MainFilter::getProperties called with null MainFilterContext");
		return obs_properties_create();
	}

	try {
		return self->getProperties();
	} catch (const std::exception &e) {
		logger->logException(e, "Failed to get properties");
	} catch (...) {
		logger->error("Failed to get properties: unknown error");
	}

	return obs_properties_create();
}

void update(void *data, obs_data_t *settings) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!data) {
		logger->error("MainFilter::update called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainFilterContext> *>(data)->get();
	if (!self) {
		logger->error("MainFilter::update called with null MainFilterContext");
		return;
	}

	try {
		self->update(settings);
	} catch (const std::exception &e) {
		logger->logException(e, "Failed to update MainFilterContext");
	} catch (...) {
		logger->error("Failed to update MainFilterContext: unknown error");
	}
}

void videoTick(void *data, float seconds) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!data) {
		logger->error("MainFilter::videoTick called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainFilterContext> *>(data)->get();
	if (!self) {
		logger->error("MainFilter::videoTick called with null MainFilterContext");
		return;
	}

	try {
		self->videoTick(seconds);
	} catch (const std::exception &e) {
		logger->logException(e, "Failed to tick MainFilterContext");
	} catch (...) {
		logger->error("Failed to tick MainFilterContext: unknown error");
	}
}

void videoRender(void *data, gs_effect_t *) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!data) {
		logger->error("MainFilter::videoRender called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainFilterContext> *>(data)->get();
	if (!self) {
		logger->error("MainFilter::videoRender called with null MainFilterContext");
		return;
	}

	try {
		self->videoRender();
		BridgeUtils::GsUnique::drain();
	} catch (const std::exception &e) {
		logger->logException(e, "Failed to render video in MainFilterContext");
	} catch (...) {
		logger->error("Failed to render video in MainFilterContext: unknown error");
	}
}

struct obs_source_frame *filterVideo(void *data, struct obs_source_frame *frame) noexcept
{
	const auto logger = g_globalContext_->logger_;

	if (!frame) {
		logger->error("MainFilter::filterVideo called with null frame");
		return nullptr;
	}

	if (!data) {
		logger->error("MainFilter::filterVideo called with null data");
		return frame;
	}

	auto self = static_cast<std::shared_ptr<MainFilterContext> *>(data)->get();
	if (!self) {
		logger->error("MainFilter::filterVideo called with null MainFilterContext");
		return frame;
	}

	try {
		return self->filterVideo(frame);
	} catch (const std::exception &e) {
		logger->logException(e, "Failed to filter video in MainFilterContext");
	} catch (...) {
		logger->error("Failed to filter video in MainFilterContext: unknown error");
	}

	return frame;
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
