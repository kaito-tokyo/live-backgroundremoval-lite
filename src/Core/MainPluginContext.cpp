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
#include <thread>

#include <obs-module.h>
#include <obs-frontend-api.h>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/gpu.h>
#else
#include <gpu.h>
#endif

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ObsLogger.hpp"
#include "../BridgeUtils/ObsUnique.hpp"

#include "DebugWindow.hpp"
#include "RenderingContext.hpp"

#include "../BridgeUtils/ObsLogger.hpp"

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
	return renderingContext ? renderingContext->region.width : 0;
}

std::uint32_t MainPluginContext::getHeight() const noexcept
{
	return renderingContext ? renderingContext->region.height : 0;
}

void MainPluginContext::getDefaults(obs_data_t *data)
{
	PluginProperty defaultProperty;

	obs_data_set_default_int(data, "computeUnit", defaultProperty.computeUnit);

	obs_data_set_default_int(data, "numThreads", defaultProperty.numThreads);

	obs_data_set_default_int(data, "filterLevel", static_cast<int>(defaultProperty.filterLevel));

	obs_data_set_default_int(data, "selfieSegmenterFps", defaultProperty.selfieSegmenterFps);

	obs_data_set_default_double(data, "guidedFilterEpsPowDb", defaultProperty.guidedFilterEpsPowDb);

	obs_data_set_default_double(data, "maskGamma", defaultProperty.maskGamma);
	obs_data_set_default_double(data, "maskLowerBoundAmpDb", defaultProperty.maskLowerBoundAmpDb);
	obs_data_set_default_double(data, "maskUpperBoundMarginAmpDb", defaultProperty.maskUpperBoundMarginAmpDb);

	obs_data_set_default_double(data, "timeAveragedFilteringAlpha", 0.1);
}

obs_properties_t *MainPluginContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	const char *updateAvailableText = obs_module_text("updateCheckerCheckingError");
	try {
		if (latestVersionFuture.valid()) {
			if (latestVersionFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				const std::string latestVersion = latestVersionFuture.get();
				logger.info("CurrentVersion: {}, Latest version: {}", PLUGIN_VERSION, latestVersion);
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

	obs_property_t *propComputeUnit = obs_properties_add_list(props, "computeUnit", obs_module_text("computeUnit"),
								  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(propComputeUnit, obs_module_text("computeUnitAuto"), ComputeUnit::kAuto);
	obs_property_list_add_int(propComputeUnit, obs_module_text("computeUnitCpuOnly"), ComputeUnit::kCpuOnly);

#if NCNN_VULKAN == 1
	int ncnnGpuCount = ncnn::get_gpu_count();
	ncnnGpuNames.resize(ncnnGpuCount);
	for (int i = 0; i < ncnnGpuCount; ++i) {
		ncnnGpuNames[i] = fmt::format("{} ({})", obs_module_text("computeUnitVulkanGpu"), i);
		obs_property_list_add_int(propComputeUnit, ncnnGpuNames[i].c_str(), ComputeUnit::kNcnnVulkanGpu + i);
	}
#endif

#if HAVE_COREML_SELFIE_SEGMENTER
	obs_property_list_add_int(propComputeUnit, obs_module_text("computeUnitCoreML"), ComputeUnit::kCoreML);
#endif

	obs_properties_add_int_slider(props, "numThreads", obs_module_text("numThreads"), 0,
				      std::thread::hardware_concurrency(), 1);

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
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelTimeAveragedFilter"),
				  static_cast<int>(FilterLevel::TimeAveragedFilter));

	obs_properties_add_int_slider(props, "selfieSegmenterFps", obs_module_text("selfieSegmenterFps"), 1, 30, 1);

	obs_properties_add_float_slider(props, "guidedFilterEpsPowDb", obs_module_text("guidedFilterEpsPowDb"), -60.0,
					-20.0, 0.1);

	obs_properties_add_float_slider(props, "maskGamma", obs_module_text("maskGamma"), 0.5, 3.0, 0.01);
	obs_properties_add_float_slider(props, "maskLowerBoundAmpDb", obs_module_text("maskLowerBoundAmpDb"), -80.0,
					-10.0, 0.1);
	obs_properties_add_float_slider(props, "maskUpperBoundMarginAmpDb",
					obs_module_text("maskUpperBoundMarginAmpDb"), -80.0, -10.0, 0.1);

	obs_properties_add_float_slider(props, "timeAveragedFilteringAlpha",
					obs_module_text("timeAveragedFilteringAlpha"), 0.0f, 1.0f, 0.01f);

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
	PluginProperty newPluginProperty;

	newPluginProperty.filterLevel = static_cast<FilterLevel>(obs_data_get_int(settings, "filterLevel"));

	newPluginProperty.selfieSegmenterFps = obs_data_get_int(settings, "selfieSegmenterFps");

	newPluginProperty.guidedFilterEpsPowDb = obs_data_get_double(settings, "guidedFilterEpsPowDb");

	newPluginProperty.maskGamma = obs_data_get_double(settings, "maskGamma");
	newPluginProperty.maskLowerBoundAmpDb = obs_data_get_double(settings, "maskLowerBoundAmpDb");
	newPluginProperty.maskUpperBoundMarginAmpDb = obs_data_get_double(settings, "maskUpperBoundMarginAmpDb");

	newPluginProperty.timeAveragedFilteringAlpha = obs_data_get_double(settings, "timeAveragedFilteringAlpha");

	std::shared_ptr<RenderingContext> _renderingContext;
	{
		std::lock_guard<std::mutex> lock(renderingContextMutex);

		bool doesRenewRenderingContext = false;

		int computeUnit = obs_data_get_int(settings, "computeUnit");
		if (pluginProperty.computeUnit != computeUnit) {
			doesRenewRenderingContext = true;
		}
		newPluginProperty.computeUnit = computeUnit;

		int numThreads = obs_data_get_int(settings, "numThreads");
		if (pluginProperty.numThreads != numThreads) {
			doesRenewRenderingContext = true;
		}
		newPluginProperty.numThreads = numThreads;

		pluginProperty = newPluginProperty;
		_renderingContext = renderingContext;

		if (_renderingContext && doesRenewRenderingContext) {
			GraphicsContextGuard graphicsContextGuard;
			std::shared_ptr<RenderingContext> newRenderingContext = createRenderingContext(
				_renderingContext->region.width, _renderingContext->region.height);
			renderingContext = newRenderingContext;
			_renderingContext = newRenderingContext;
			GsUnique::drain();
		}
	}

	if (_renderingContext) {
		_renderingContext->applyPluginProperty(pluginProperty);
	}
}

void MainPluginContext::activate() {}

void MainPluginContext::deactivate() {}

void MainPluginContext::show() {}

void MainPluginContext::hide() {}

void MainPluginContext::videoTick(float seconds)
{
	if (obs_source_t *const parent = obs_filter_get_parent(source)) {
		if (!obs_source_active(parent)) {
			return;
		}
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

	std::shared_ptr<RenderingContext> _renderingContext;
	{
		std::lock_guard<std::mutex> lock(renderingContextMutex);
		if (!renderingContext || renderingContext->region.width != targetWidth ||
		    renderingContext->region.height != targetHeight) {
			GraphicsContextGuard graphicsContextGuard;
			renderingContext = createRenderingContext(targetWidth, targetHeight);
			GsUnique::drain();
		}
		_renderingContext = renderingContext;
	}

	if (_renderingContext) {
		_renderingContext->videoTick(seconds);
	}
}

void MainPluginContext::videoRender()
{
	if (obs_source_t *const parent = obs_filter_get_parent(source)) {
		if (!obs_source_active(parent) || !obs_source_showing(parent)) {
			// Draw nothing to prevent unexpected background disclosure
			return;
		}
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
	if (obs_source_t *const parent = obs_filter_get_parent(source)) {
		if (!obs_source_active(parent) || !obs_source_showing(parent)) {
			return frame;
		}
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
	PluginConfig pluginConfig(PluginConfig::load());
	auto renderingContext = std::make_shared<RenderingContext>(
		source, logger, mainEffect, selfieSegmenterTaskQueue, pluginConfig, pluginProperty.subsamplingRate,
		targetWidth, targetHeight, pluginProperty.computeUnit, pluginProperty.numThreads);
	renderingContext->applyPluginProperty(pluginProperty);
	return renderingContext;
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
