/*
Background Removal Lite
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
#include <obs-frontend-api.h>

#include <obs-bridge-utils/ObsLogger.hpp>

#include "DebugWindow.hpp"
#include "RenderingContext.hpp"

using namespace kaito_tokyo::obs_bridge_utils;

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

MainPluginContext::MainPluginContext(obs_data_t *settings, obs_source_t *_source)
	: source{_source},
	  logger("[" PLUGIN_NAME "] "),
	  mainEffect(unique_obs_module_file("effects/main.effect"), logger),
	  selfieSegmenterTaskQueue(logger, 1)
{
	selfieSegmenterNet.opt.num_threads = 2;
	selfieSegmenterNet.opt.use_local_pool_allocator = true;
	selfieSegmenterNet.opt.openmp_blocktime = 1;

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

void MainPluginContext::startup() noexcept {}

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

	obs_data_set_default_int(data, "selfieSegmenterFps", 10);

	obs_data_set_default_double(data, "gfEpsDb", -40.0);

	obs_data_set_default_double(data, "maskGamma", 2.5);
	obs_data_set_default_double(data, "maskLowerBoundDb", -25.0);
	obs_data_set_default_double(data, "maskUpperBoundMarginDb", -25.0);
}

obs_properties_t *MainPluginContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	const char *updateAvailableText = obs_module_text("updateCheckerPluginIsLatest");
	std::optional<std::string> latestVersion = getLatestVersion();
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

	obs_properties_add_int_slider(props, "selfieSegmenterFps", obs_module_text("selfieSegmenterFps"), 1, 30, 1);

	obs_properties_add_float_slider(props, "gfEpsDb", obs_module_text("gfEpsFb"), -60.0, -20.0, 0.1);

	obs_properties_add_float_slider(props, "maskGamma", obs_module_text("maskGamma"), 0.5, 3.0, 0.01);
	obs_properties_add_float_slider(props, "maskLowerBoundDb", obs_module_text("maskLowerBoundDb"), -80.0, -10.0,
					0.1);
	obs_properties_add_float_slider(props, "maskUpperBoundMarginDb", obs_module_text("maskUpperBoundMarginDb"),
					-80.0, -10.0, 0.1);

	obs_properties_add_button2(
		props, "showDebugWindow", obs_module_text("showDebugWindow"),
		[](obs_properties_t *, obs_property_t *, void *data) {
			auto _this = static_cast<MainPluginContext *>(data);
			auto parent = static_cast<QWidget *>(obs_frontend_get_main_window());
			_this->debugWindow = std::make_unique<DebugWindow>(_this->weak_from_this(), parent);
			_this->debugWindow->show();
			return false;
		},
		this);

	return props;
}

void MainPluginContext::update(obs_data_t *settings)
{
	preset.filterLevel = static_cast<FilterLevel>(obs_data_get_int(settings, "filterLevel"));

	preset.selfieSegmenterFps = obs_data_get_int(settings, "selfieSegmenterFps");

	preset.gfEpsDb = obs_data_get_double(settings, "gfEpsDb");
	preset.gfEps = Preset::dbToLinearPow(preset.gfEpsDb);

	preset.maskGamma = obs_data_get_double(settings, "maskGamma");
	preset.maskLowerBoundDb = obs_data_get_double(settings, "maskLowerBoundDb");
	preset.maskLowerBound = Preset::dbToLinearAmp(preset.maskLowerBoundDb);
	preset.maskUpperBoundMarginDb = obs_data_get_double(settings, "maskUpperBoundMarginDb");
	preset.maskUpperBound = 1.0 - Preset::dbToLinearAmp(preset.maskUpperBoundMarginDb);
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

	if (nextRenderingContext) {
		nextRenderingContext->videoRender();
	}

	if (debugWindow) {
		debugWindow->videoRender();
	}
}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
try {
	if (frame->width == 0 || frame->height == 0) {
		renderingContext.reset();
		return frame;
	}

	if (frameCountBeforeContextSwitch > 0) {
		--frameCountBeforeContextSwitch;
		if (frameCountBeforeContextSwitch == 0 && nextRenderingContext) {
			graphics_context_guard guard;
			renderingContext = std::move(nextRenderingContext);
			nextRenderingContext.reset();
			logger.info("Switched to new rendering context");
			gs_unique::drain();
		}
	}

	if (!renderingContext || renderingContext->width != frame->width || renderingContext->height != frame->height) {
		graphics_context_guard guard;
		nextRenderingContext = std::make_shared<RenderingContext>(
			source, logger, mainEffect, selfieSegmenterNet, selfieSegmenterTaskQueue, frame->width,
			frame->height, preset.filterLevel, preset.selfieSegmenterFps, preset.gfEps, preset.maskGamma,
			preset.maskLowerBound, preset.maskUpperBound);
		frameCountBeforeContextSwitch = 1;
		gs_unique::drain();
	}

	if (renderingContext) {
		return renderingContext->filterVideo(frame);
	} else {
		return frame;
	}
} catch (const std::exception &e) {
	logger.error("Failed to create rendering context: {}", e.what());
	return frame;
} catch (...) {
	logger.error("Failed to create rendering context: unknown error");
	return frame;
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
