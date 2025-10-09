/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "MainPluginContext.h"

#include <future>
#include <stdexcept>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ObsLogger.hpp"
#include "../BridgeUtils/ObsUnique.hpp"

#include "DebugWindow.hpp"
#include "RenderingContext.hpp"

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

MainPluginContext::MainPluginContext(obs_data_t *settings, obs_source_t *_source,
				     std::shared_future<std::string> _latestVersionFuture,
				     const BridgeUtils::ILogger &_logger)
	: source{_source},
	  logger(_logger),
	  mainEffect(unique_obs_module_file("effects/main.effect"), logger),
	  latestVersionFuture(_latestVersionFuture),
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
	if (DebugWindow *_debugWindow = debugWindow.load()) {
		_debugWindow->close();
	}

	{
		std::lock_guard<std::mutex> lock(renderingContextMutex);
		renderingContext.reset();
	}

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

	const char *updateAvailableText = obs_module_text("updateCheckerCheckingError");
	try {
		if (latestVersionFuture.valid()) {
			if (latestVersionFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				const std::string latestVersion = latestVersionFuture.get();
				logger.info("Latest version: {}", latestVersion);
				if (latestVersion == PLUGIN_VERSION) {
					updateAvailableText = obs_module_text("updateCheckerPluginIsLatest");
				} else {
					updateAvailableText = obs_module_text("updateCheckerUpdateAvailable");
				}
			} else {
				updateAvailableText = obs_module_text("updateCheckerChecking");
			}
		}
	} catch (const std::exception &e) {
		logger.error("Failed to check for updates: {}", e.what());
	} catch (...) {
		logger.error("Failed to check for updates: unknown error");
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
			auto self = static_cast<MainPluginContext *>(data)->shared_from_this();

			if (DebugWindow *_debugWindow = self->debugWindow.load()) {
				_debugWindow->show();
				_debugWindow->raise();
				_debugWindow->activateWindow();
			} else {
				auto parent = static_cast<QWidget *>(obs_frontend_get_main_window());
				DebugWindow *newDebugWindow = new DebugWindow(self->weak_from_this(), parent);
				newDebugWindow->setAttribute(Qt::WA_DeleteOnClose);

				QObject::connect(newDebugWindow, &QDialog::destroyed,
						 [self]() { self->setDebugWindowNull(); });

				self->debugWindow.store(newDebugWindow);

				newDebugWindow->show();
			}

			return false;
		},
		this);

	return props;
}

void MainPluginContext::update(obs_data_t *settings)
{
	pluginProperty.filterLevel = static_cast<FilterLevel>(obs_data_get_int(settings, "filterLevel"));

	pluginProperty.selfieSegmenterFps = obs_data_get_int(settings, "selfieSegmenterFps");

	pluginProperty.gfEpsDb = obs_data_get_double(settings, "gfEpsDb");
	pluginProperty.gfEps = PluginProperty::dbToLinearPow(pluginProperty.gfEpsDb);

	pluginProperty.maskGamma = obs_data_get_double(settings, "maskGamma");
	pluginProperty.maskLowerBoundDb = obs_data_get_double(settings, "maskLowerBoundDb");
	pluginProperty.maskLowerBound = PluginProperty::dbToLinearAmp(pluginProperty.maskLowerBoundDb);
	pluginProperty.maskUpperBoundMarginDb = obs_data_get_double(settings, "maskUpperBoundMarginDb");
	pluginProperty.maskUpperBound = 1.0 - PluginProperty::dbToLinearAmp(pluginProperty.maskUpperBoundMarginDb);

	if (auto _renderingContext = getRenderingContext()) {
		_renderingContext->setPluginProperty(pluginProperty);
	}
}

void MainPluginContext::activate()
{
	MainPluginState current = mainPluginState.load();
	MainPluginState desired;
	do {
		desired = current;
		desired.isActive = true;
	} while (!mainPluginState.compare_exchange_weak(current, desired, std::memory_order_release, std::memory_order_relaxed));
}

void MainPluginContext::deactivate()
{
	MainPluginState current = mainPluginState.load();
	MainPluginState desired;
	do {
		desired = current;
		desired.isActive = false;
	} while (!mainPluginState.compare_exchange_weak(current, desired, std::memory_order_release, std::memory_order_relaxed));
}

void MainPluginContext::show()
{
	MainPluginState current = mainPluginState.load();
	MainPluginState desired;
	do {
		desired = current;
		desired.isVisible = true;
	} while (!mainPluginState.compare_exchange_weak(current, desired, std::memory_order_release, std::memory_order_relaxed));
}

void MainPluginContext::hide()
{
	MainPluginState current = mainPluginState.load();
	MainPluginState desired;
	do {
		desired = current;
		desired.isVisible = false;
	} while (!mainPluginState.compare_exchange_weak(current, desired, std::memory_order_release, std::memory_order_relaxed));
}

void MainPluginContext::videoTick(float seconds)
{
	MainPluginState _mainPluginState = mainPluginState.load();

	if (!_mainPluginState.isActive) {
		std::lock_guard<std::mutex> lock(renderingContextMutex);
		renderingContext.reset();
		return;
	}

	obs_source_t *target = obs_filter_get_target(source);
	uint32_t targetWidth = obs_source_get_width(target);
	uint32_t targetHeight = obs_source_get_height(target);

	if (targetWidth == 0 || targetHeight == 0) {
		targetWidth = obs_source_get_base_width(target);
		targetHeight = obs_source_get_base_height(target);
	}

	if (targetWidth == 0 || targetHeight == 0) {
		logger.debug("Target source has zero width or height, skipping video tick");
		return;
	}

	auto _renderingContext = getRenderingContext();
	if (!_renderingContext || _renderingContext->width != targetWidth ||
	    _renderingContext->height != targetHeight) {
		GraphicsContextGuard graphicsContextGuard;
		_renderingContext = createRenderingContext(targetWidth, targetHeight);
		{
			std::lock_guard<std::mutex> lock(renderingContextMutex);
			renderingContext = _renderingContext;
		}
		GsUnique::drain();
	}

	if (_renderingContext) {
		_renderingContext->videoTick(seconds);
	}
}

void MainPluginContext::videoRender()
{
	MainPluginState _mainPluginState = mainPluginState.load();

	if (!_mainPluginState.isActive || !_mainPluginState.isVisible) {
		return;
	}

	if (auto _renderingContext = getRenderingContext()) {
		_renderingContext->videoRender();
	}

	if (DebugWindow *_debugWindow = debugWindow.load()) {
		_debugWindow->videoRender();
	}
}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
try {
	MainPluginState _mainPluginState = mainPluginState.load();

	if (!_mainPluginState.isActive || !_mainPluginState.isVisible) {
		return frame;
	}

	if (auto _renderingContext = getRenderingContext()) {
		return _renderingContext->filterVideo(frame);
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

std::shared_ptr<RenderingContext> MainPluginContext::createRenderingContext(std::uint32_t targetWidth,
									    std::uint32_t targetHeight)
{
	PluginConfig defaultPluginConfig;

	auto renderingContext = std::make_shared<RenderingContext>(source, logger, mainEffect, selfieSegmenterNet,
								   selfieSegmenterTaskQueue, defaultPluginConfig,
								   subsamplingRate, targetWidth, targetHeight);
	renderingContext->setPluginProperty(pluginProperty);
	return renderingContext;
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
