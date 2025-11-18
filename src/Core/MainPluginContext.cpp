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

#include <GsUnique.hpp>
#include <ObsLogger.hpp>
#include <ObsUnique.hpp>

#include "DebugWindow.hpp"
#include "RenderingContext.hpp"

using namespace KaitoTokyo::Logger;
using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

MainPluginContext::MainPluginContext(obs_data_t *settings, obs_source_t *source,
				     std::shared_future<std::string> latestVersionFuture, const ILogger &logger)
	: source_{source},
	  logger_(logger),
	  mainEffect_(logger_, unique_obs_module_file("effects/main.effect")),
	  latestVersionFuture_(latestVersionFuture),
	  selfieSegmenterTaskQueue_(logger_, 1)
{
	update(settings);
}

void MainPluginContext::shutdown() noexcept
{
	{
		std::lock_guard<std::mutex> lock(debugWindowMutex_);
		if (debugWindow_) {
			try {
				debugWindow_->close();
			} catch (...) {
				// Ignore
			}
		}
	}

	selfieSegmenterTaskQueue_.shutdown();
}

MainPluginContext::~MainPluginContext() noexcept {}

std::uint32_t MainPluginContext::getWidth() const noexcept
{
	auto renderingContext = getRenderingContext();
	return renderingContext ? renderingContext->region_.width : 0;
}

std::uint32_t MainPluginContext::getHeight() const noexcept
{
	auto renderingContext = getRenderingContext();
	return renderingContext ? renderingContext->region_.height : 0;
}

void MainPluginContext::getDefaults(obs_data_t *data)
{
	PluginProperty defaultProperty;

	obs_data_set_default_int(data, "filterLevel", static_cast<int>(defaultProperty.filterLevel));

	obs_data_set_default_double(data, "motionIntensityThresholdPowDb",
				    defaultProperty.motionIntensityThresholdPowDb);

	obs_data_set_default_double(data, "timeAveragedFilteringAlpha", defaultProperty.timeAveragedFilteringAlpha);

	obs_data_set_default_bool(data, "advancedSettings", false);

	obs_data_set_default_int(data, "numThreads", defaultProperty.numThreads);

	obs_data_set_default_double(data, "guidedFilterEpsPowDb", defaultProperty.guidedFilterEpsPowDb);

	obs_data_set_default_double(data, "maskGamma", defaultProperty.maskGamma);
	obs_data_set_default_double(data, "maskLowerBoundAmpDb", defaultProperty.maskLowerBoundAmpDb);
	obs_data_set_default_double(data, "maskUpperBoundMarginAmpDb", defaultProperty.maskUpperBoundMarginAmpDb);
}

obs_properties_t *MainPluginContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	// Update notifier
	const char *updateAvailableText = obs_module_text("updateCheckerCheckingError");
	try {
		if (latestVersionFuture_.valid()) {
			if (latestVersionFuture_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				const std::string latestVersion = latestVersionFuture_.get();
				logger_.info("CurrentVersion: {}, Latest version: {}", PLUGIN_VERSION, latestVersion);
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
		logger_.error("Failed to check for updates: {}", e.what());
	} catch (...) {
		logger_.error("Failed to check for updates: unknown error");
	}
	obs_properties_add_text(props, "isUpdateAvailable", updateAvailableText, OBS_TEXT_INFO);

	// Debug button
	obs_properties_add_button2(
		props, "showDebugWindow", obs_module_text("showDebugWindow"),
		[](obs_properties_t *, obs_property_t *, void *data) {
			auto this_ = static_cast<MainPluginContext *>(data);
			DebugWindow *windowToShow = nullptr;

			{
				std::lock_guard<std::mutex> lock(this_->debugWindowMutex_);
				if (this_->debugWindow_) {
					windowToShow = this_->debugWindow_;
				} else {
					auto parent = static_cast<QWidget *>(obs_frontend_get_main_window());
					auto newDebugWindow = new DebugWindow(this_->weak_from_this(), parent);

					newDebugWindow->setAttribute(Qt::WA_DeleteOnClose);

					QObject::connect(newDebugWindow, &QDialog::destroyed,
							 [newDebugWindow, weakSelf = this_->weak_from_this()]() {
								 if (auto self = weakSelf.lock()) {
									 std::lock_guard<std::mutex> lock(
										 self->debugWindowMutex_);
									 if (self->debugWindow_ == newDebugWindow) {
										 self->debugWindow_ = nullptr;
									 }
								 }
							 });
					windowToShow = newDebugWindow;
					this_->debugWindow_ = newDebugWindow;
				}
			}

			if (windowToShow) {
				windowToShow->show();
				windowToShow->raise();
				windowToShow->activateWindow();
			}

			return false;
		},
		this);

	// Filter level
	obs_property_t *propFilterLevel = obs_properties_add_list(props, "filterLevel", obs_module_text("filterLevel"),
								  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelDefault"),
				  static_cast<int>(FilterLevel::Default));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelPassthrough"),
				  static_cast<int>(FilterLevel::Passthrough));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelSegmentation"),
				  static_cast<int>(FilterLevel::Segmentation));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelMotionIntensityThresholding"),
				  static_cast<int>(FilterLevel::MotionIntensityThresholding));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelGuidedFilter"),
				  static_cast<int>(FilterLevel::GuidedFilter));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelTimeAveragedFilter"),
				  static_cast<int>(FilterLevel::TimeAveragedFilter));

	// Motion intensity threshold
	obs_properties_add_float_slider(props, "motionIntensityThresholdPowDb",
					obs_module_text("motionIntensityThresholdPowDb"), -100.0, 0.0, 0.1);

	// Time-averaged filtering
	obs_properties_add_float_slider(props, "timeAveragedFilteringAlpha",
					obs_module_text("timeAveragedFilteringAlpha"), 0.0f, 1.0f, 0.01f);

	// Advanced settings group
	obs_properties_t *propsAdvancedSettings = obs_properties_create();
	obs_properties_add_group(props, "advancedSettings", obs_module_text("advancedSettings"), OBS_GROUP_CHECKABLE,
				 propsAdvancedSettings);

	// Number of threads
	obs_properties_add_int_slider(propsAdvancedSettings, "numThreads", obs_module_text("numThreads"), 0, 16, 2);

	// Guided filter
	obs_properties_add_float_slider(propsAdvancedSettings, "guidedFilterEpsPowDb",
					obs_module_text("guidedFilterEpsPowDb"), -60.0, -20.0, 0.1);

	// Mask application
	obs_properties_add_float_slider(propsAdvancedSettings, "maskGamma", obs_module_text("maskGamma"), 0.5, 3.0,
					0.01);
	obs_properties_add_float_slider(propsAdvancedSettings, "maskLowerBoundAmpDb",
					obs_module_text("maskLowerBoundAmpDb"), -80.0, -10.0, 0.1);
	obs_properties_add_float_slider(propsAdvancedSettings, "maskUpperBoundMarginAmpDb",
					obs_module_text("maskUpperBoundMarginAmpDb"), -80.0, -10.0, 0.1);

	return props;
}

void MainPluginContext::update(obs_data_t *settings)
{
	bool advancedSettingsEnabled = obs_data_get_bool(settings, "advancedSettings");

	PluginProperty newPluginProperty;

	newPluginProperty.filterLevel = static_cast<FilterLevel>(obs_data_get_int(settings, "filterLevel"));

	newPluginProperty.motionIntensityThresholdPowDb =
		obs_data_get_double(settings, "motionIntensityThresholdPowDb");

	newPluginProperty.timeAveragedFilteringAlpha = obs_data_get_double(settings, "timeAveragedFilteringAlpha");

	if (advancedSettingsEnabled) {
		newPluginProperty.guidedFilterEpsPowDb = obs_data_get_double(settings, "guidedFilterEpsPowDb");

		newPluginProperty.maskGamma = obs_data_get_double(settings, "maskGamma");
		newPluginProperty.maskLowerBoundAmpDb = obs_data_get_double(settings, "maskLowerBoundAmpDb");
		newPluginProperty.maskUpperBoundMarginAmpDb =
			obs_data_get_double(settings, "maskUpperBoundMarginAmpDb");
	}

	std::shared_ptr<RenderingContext> renderingContext;
	{
		std::lock_guard<std::mutex> lock(renderingContextMutex_);

		bool doesRenewRenderingContext = false;

		int numThreads;
		if (advancedSettingsEnabled) {
			numThreads = obs_data_get_int(settings, "numThreads");
		} else {
			numThreads = newPluginProperty.numThreads;
		}

		if (pluginProperty_.numThreads != numThreads) {
			doesRenewRenderingContext = true;
		}
		newPluginProperty.numThreads = numThreads;

		pluginProperty_ = newPluginProperty;
		renderingContext = renderingContext_;

		if (renderingContext && doesRenewRenderingContext) {
			GraphicsContextGuard graphicsContextGuard;
			std::shared_ptr<RenderingContext> newRenderingContext = createRenderingContext(
				renderingContext->region_.width, renderingContext->region_.height);
			renderingContext = newRenderingContext;
			GsUnique::drain();
		}
	}

	if (renderingContext) {
		renderingContext->applyPluginProperty(pluginProperty_);
	}
}

void MainPluginContext::videoTick(float seconds)
{
	obs_source_t *const parent = obs_filter_get_parent(source_);
	if (!parent) {
		logger_.debug("No parent source found, skipping video tick");
		return;
	} else if (!obs_source_active(parent)) {
		logger_.debug("Parent source is not active, skipping video tick");
		return;
	}

	obs_source_t *const target = obs_filter_get_target(source_);
	if (!target) {
		logger_.debug("No target source found, skipping video tick");
		return;
	}

	uint32_t targetWidth = obs_source_get_base_width(target);
	uint32_t targetHeight = obs_source_get_base_height(target);

	if (targetWidth == 0 || targetHeight == 0) {
		logger_.debug("Target source has zero width or height, skipping video tick");
		return;
	}

	std::shared_ptr<RenderingContext> renderingContext;
	{
		std::lock_guard<std::mutex> lock(renderingContextMutex_);
		renderingContext = renderingContext_;
		if (!renderingContext || renderingContext->region_.width != targetWidth ||
		    renderingContext->region_.height != targetHeight) {
			GraphicsContextGuard graphicsContextGuard;
			renderingContext_ = createRenderingContext(targetWidth, targetHeight);
			GsUnique::drain();
			renderingContext = renderingContext_;
		}
	}

	if (renderingContext) {
		renderingContext->videoTick(seconds);
	}
}

void MainPluginContext::videoRender()
{
	obs_source_t *const parent = obs_filter_get_parent(source_);
	if (!parent) {
		logger_.debug("No parent source found, skipping video render");
		// Draw nothing to prevent unexpected background disclosure
		return;
	} else if (!obs_source_active(parent)) {
		logger_.debug("Parent source is not active, skipping video render");
		// Draw nothing to prevent unexpected background disclosure
		return;
	} else if (!obs_source_showing(parent)) {
		logger_.debug("Parent source is not showing, skipping video render");
		// Draw nothing to prevent unexpected background disclosure
		return;
	}

	if (auto _renderingContext = getRenderingContext()) {
		_renderingContext->videoRender();
	}

	{
		std::lock_guard<std::mutex> lock(debugWindowMutex_);

		if (debugWindow_) {
			debugWindow_->videoRender();
		}
	}
}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
try {
	if (obs_source_t *const parent = obs_filter_get_parent(source_)) {
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
	logger_.error("Failed to create rendering context: {}", e.what());
	return frame;
} catch (...) {
	logger_.error("Failed to create rendering context: unknown error");
	return frame;
}

std::shared_ptr<RenderingContext> MainPluginContext::createRenderingContext(std::uint32_t targetWidth,
									    std::uint32_t targetHeight)
{
	PluginConfig pluginConfig(PluginConfig::load(logger_));

	auto renderingContext = std::make_shared<RenderingContext>(source_, logger_, mainEffect_,
								   selfieSegmenterTaskQueue_, pluginConfig,
								   pluginProperty_.subsamplingRate, targetWidth,
								   targetHeight, pluginProperty_.numThreads);

	renderingContext->applyPluginProperty(pluginProperty_);

	return renderingContext;
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
