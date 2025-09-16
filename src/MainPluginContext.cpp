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

#include <obs-module.h>

#include <obs-bridge-utils/ObsLogger.hpp>

using namespace kaito_tokyo::obs_bridge_utils;

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

MainPluginContext::MainPluginContext(obs_data_t *settings, obs_source_t *_source)
	: source{_source},
	  logger("[" PLUGIN_NAME "] "),
	  mainEffect(unique_obs_module_file("effects/main.effect"), logger),
	  updateChecker(logger),
	  selfieSegmenterTaskQueue(logger, 1)
{
	unique_bfree_char_t paramPath = unique_obs_module_file("models/mediapipe_selfie_segmentation_int8.ncnn.param");
	if (!paramPath) {
		throw std::runtime_error("Failed to find model param file");
	}
	unique_bfree_char_t binPath = unique_obs_module_file("models/mediapipe_selfie_segmentation_int8.ncnn.bin");
	if (!binPath) {
		throw std::runtime_error("Failed to find model bin file");
	}

	if (selfieSegmenterNet.load_param(paramPath.get()) != 0) {
		throw std::runtime_error("Failed to load model param");
	}
	if (selfieSegmenterNet.load_model(binPath.get()) != 0) {
		throw std::runtime_error("Failed to load model bin");
	}

	update(settings);
}

void MainPluginContext::startup() noexcept
{
	futureLatestVersion = std::async(std::launch::async, [self = weak_from_this()]() -> std::optional<std::string> {
		if (auto s = self.lock()) {
			return s.get()->updateChecker.fetch();
		} else {
			blog(LOG_INFO, "MainPluginContext has been destroyed, skipping update check");
			return std::nullopt;
		}
	});
}

void MainPluginContext::shutdown() noexcept
{
	renderingContext.reset();
	selfieSegmenterTaskQueue.shutdown();
}

MainPluginContext::~MainPluginContext() noexcept {}

std::uint32_t MainPluginContext::getWidth() const noexcept
{
	return renderingContext ? renderingContext->width : 0;
}

std::uint32_t MainPluginContext::getHeight() const noexcept
{
	return renderingContext ? renderingContext->height : 0;
}

void MainPluginContext::getDefaults(obs_data_t *data)
{
	obs_data_set_default_int(data, "filterLevel", static_cast<int>(FilterLevel::Default));
	obs_data_set_default_int(data, "gfRadius", 8);
	obs_data_set_default_double(data, "gfEps", 0.0004);
	obs_data_set_default_int(data, "gfSubsamplingRate", 4);
}

obs_properties_t *MainPluginContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	const char *updateAvailableText = obs_module_text("updateCheckerPluginIsLatest");
	std::optional<std::string> latestVersion = getLatestVersion();
	if (latestVersion.has_value() && updateChecker.isUpdateAvailable(latestVersion.value(), PLUGIN_VERSION)) {
		updateAvailableText = obs_module_text("updateCheckerUpdateAvailable");
	}

	obs_properties_add_text(props, "isUpdateAvailable", updateAvailableText, OBS_TEXT_INFO);

	obs_property_t *propFilterLevel = obs_properties_add_list(props, "filterLevel", obs_module_text("filterLevel"),
								  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelDefault"),
				  static_cast<int>(FilterLevel::Default));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelPassthrough"),
				  static_cast<int>(FilterLevel::Passthrough));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelSegmentation"),
				  static_cast<int>(FilterLevel::Segmentation));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelGuidedFilter"),
				  static_cast<int>(FilterLevel::GuidedFilter));

	obs_properties_add_int_slider(props, "gfRadius", obs_module_text("gfRadius"), 0, 16, 2);
	obs_properties_add_float_slider(props, "gfEps", obs_module_text("gfEps"), 0.000001f, 0.0004f, 0.000001f);
	obs_properties_add_int_slider(props, "gfSubsamplingRate", obs_module_text("gfSubsamplingRate"), 1, 16, 1);

	return props;
}

void MainPluginContext::update(obs_data_t *settings)
{
	preset.filterLevel = static_cast<FilterLevel>(obs_data_get_int(settings, "filterLevel"));
	preset.gfRadius = static_cast<int>(obs_data_get_int(settings, "gfRadius"));
	preset.gfEps = static_cast<float>(obs_data_get_double(settings, "gfEps"));
	preset.gfSubsamplingRate = static_cast<int>(obs_data_get_int(settings, "gfSubsamplingRate"));
}

void MainPluginContext::activate() {}

void MainPluginContext::deactivate() {}

void MainPluginContext::show() {}

void MainPluginContext::hide() {}

void MainPluginContext::videoTick(float seconds)
{
	if (!renderingContext) {
		logger.debug("Rendering context is not initialized, skipping video tick");
		return;
	}

	renderingContext->videoTick(seconds);
}

void MainPluginContext::videoRender()
{
	if (!renderingContext) {
		logger.debug("Rendering context is not initialized, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	renderingContext->videoRender();
}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
try {
	if (frame->width == 0 || frame->height == 0) {
		renderingContext.reset();
		return frame;
	}
	if (!renderingContext || renderingContext->width != frame->width || renderingContext->height != frame->height ||
	    renderingContext->filterLevel != preset.filterLevel || renderingContext->gfRadius != preset.gfRadius ||
	    renderingContext->gfEps != preset.gfEps ||
	    renderingContext->gfSubsamplingRate != preset.gfSubsamplingRate) {
		graphics_context_guard guard;
		renderingContext = std::make_shared<RenderingContext>(
			source, logger, mainEffect, selfieSegmenterNet, selfieSegmenterTaskQueue, frame->width,
			frame->height, preset.filterLevel, preset.gfRadius, preset.gfEps, preset.gfSubsamplingRate);
		gs_unique::drain();
	}

	return frame;
} catch (const std::exception &e) {
	logger.error("Failed to create rendering context: {}", e.what());
	return frame;
} catch (...) {
	logger.error("Failed to create rendering context: unknown error");
	return frame;
}

std::optional<std::string> MainPluginContext::getLatestVersion() const
{
	if (!futureLatestVersion.valid()) {
		return std::nullopt;
	}

	if (futureLatestVersion.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
		return std::nullopt;
	}

	return futureLatestVersion.get();
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
