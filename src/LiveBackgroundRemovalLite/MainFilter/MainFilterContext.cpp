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

#include "MainFilterContext.hpp"

#include <future>
#include <stdexcept>
#include <thread>

#include <QMainWindow>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <GsUnique.hpp>
#include <ObsLogger.hpp>
#include <ObsUnique.hpp>
#include <PluginConfigDialog.hpp>

#include "DebugWindow.hpp"
#include "RenderingContext.hpp"

using namespace KaitoTokyo::Logger;
using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

MainFilterContext::MainFilterContext(obs_data_t *settings, obs_source_t *source,
				     std::shared_ptr<Global::PluginConfig> pluginConfig,
				     std::shared_ptr<Global::GlobalContext> globalContext)
	: source_{source},
	  pluginConfig_{std::move(pluginConfig)},
	  globalContext_{std::move(globalContext)},
	  logger_(globalContext_->logger_),
	  mainEffect_(logger_, unique_obs_module_file("effects/main.effect")),
	  selfieSegmenterTaskQueue_(logger_, 1)
{
	update(settings);
}

void MainFilterContext::shutdown() noexcept
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

	{
		std::lock_guard<std::mutex> lock(renderingContextMutex_);
		renderingContext_.reset();
	}
	selfieSegmenterTaskQueue_.shutdown();
}

MainFilterContext::~MainFilterContext() noexcept {}

std::uint32_t MainFilterContext::getWidth() const noexcept
{
	auto renderingContext = getRenderingContext();
	return renderingContext ? renderingContext->region_.width : 0;
}

std::uint32_t MainFilterContext::getHeight() const noexcept
{
	auto renderingContext = getRenderingContext();
	return renderingContext ? renderingContext->region_.height : 0;
}

void MainFilterContext::getDefaults(obs_data_t *data)
{
	PluginProperty defaultProperty;

	obs_data_set_default_int(data, "filterLevel", static_cast<int>(defaultProperty.filterLevel));

	obs_data_set_default_double(data, "motionIntensityThresholdPowDb",
				    defaultProperty.motionIntensityThresholdPowDb);

	obs_data_set_default_double(data, "timeAveragedFilteringAlpha", defaultProperty.timeAveragedFilteringAlpha);

	obs_data_set_default_bool(data, "advancedSettings", false);

	obs_data_set_default_int(data, "numThreads", defaultProperty.numThreads);

	obs_data_set_default_double(data, "guidedFilterEpsPowDb", defaultProperty.guidedFilterEpsPowDb);

	// Removed enableDynamicRoi
	obs_data_set_default_bool(data, "enableCenterFrame", false);

	obs_data_set_default_double(data, "maskGamma", defaultProperty.maskGamma);
	obs_data_set_default_double(data, "maskLowerBoundAmpDb", defaultProperty.maskLowerBoundAmpDb);
	obs_data_set_default_double(data, "maskUpperBoundMarginAmpDb", defaultProperty.maskUpperBoundMarginAmpDb);
}

obs_properties_t *MainFilterContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	// Update notifier
	if (!pluginConfig_->disableAutoCheckForUpdate) {
		const char *updateAvailableText = obs_module_text("updateCheckerCheckingError");
		try {
			std::string latestVersion = globalContext_->getLatestVersion();
			if (!latestVersion.empty()) {
				if (latestVersion == globalContext_->pluginVersion_) {
					updateAvailableText = obs_module_text("updateCheckerPluginIsLatest");
				} else {
					updateAvailableText = obs_module_text("updateCheckerUpdateAvailable");
				}
			}
		} catch (const std::exception &e) {
			logger_->error("Failed to check for updates: {}", e.what());
		} catch (...) {
			logger_->error("Failed to check for updates: unknown error");
		}
		obs_properties_add_text(props, "isUpdateAvailable", updateAvailableText, OBS_TEXT_INFO);
	}

	// Debug button
	obs_properties_add_button2(
		props, "showDebugWindow", obs_module_text("showDebugWindow"),
		[](obs_properties_t *, obs_property_t *, void *data) {
			auto this_ = static_cast<MainFilterContext *>(data);
			DebugWindow *windowToShow = nullptr;

			{
				std::lock_guard<std::mutex> lock(this_->debugWindowMutex_);
				if (this_->debugWindow_) {
					windowToShow = this_->debugWindow_;
				} else {
					auto parent = static_cast<QWidget *>(obs_frontend_get_main_window());
					auto newDebugWindow = new DebugWindow(this_->weak_from_this(), parent);

					newDebugWindow->setAttribute(Qt::WA_DeleteOnClose);

					QObject::connect(newDebugWindow, &QDialog::finished,
							 [newDebugWindow, weakSelf = this_->weak_from_this()](int) {
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

	// Center Frame
	obs_properties_add_bool(props, "enableCenterFrame", obs_module_text("enableCenterFrame"));

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

	// Global config dialog button
	obs_properties_add_button2(
		props, "openGlobalConfigDialog", obs_module_text("openGlobalConfigDialog"),
		[](obs_properties_t *, obs_property_t *, void *data) {
			QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
			if (!mainWindow)
				return false;

			MainFilterContext *self = static_cast<MainFilterContext *>(data);
			if (!self)
				return false;

			Global::PluginConfigDialog dialog(self->pluginConfig_, mainWindow);
			dialog.exec();
			return false;
		},
		this);

	return props;
}

void MainFilterContext::update(obs_data_t *settings)
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

	newPluginProperty.enableCenterFrame = obs_data_get_bool(settings, "enableCenterFrame");

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

void MainFilterContext::videoTick(float seconds)
{
	obs_source_t *const parent = obs_filter_get_parent(source_);
	if (!parent) {
		logger_->debug("No parent source found, skipping video tick");
		return;
	} else if (!obs_source_active(parent)) {
		logger_->debug("Parent source is not active, skipping video tick");
		return;
	}

	obs_source_t *const target = obs_filter_get_target(source_);
	if (!target) {
		logger_->debug("No target source found, skipping video tick");
		return;
	}

	uint32_t targetWidth = obs_source_get_base_width(target);
	uint32_t targetHeight = obs_source_get_base_height(target);

	std::shared_ptr<RenderingContext> renderingContext;
	{
		std::lock_guard<std::mutex> lock(renderingContextMutex_);

		if (targetWidth == 0 || targetHeight == 0) {
			logger_->debug(
				"Target source has zero width or height, skipping video tick and destroying rendering context");
			renderingContext_.reset();
			return;
		}

		const std::uint32_t minSize = 2 * static_cast<std::uint32_t>(pluginProperty_.subsamplingRate);
		if (targetWidth < minSize || targetHeight < minSize) {
			logger_->debug(
				"Target source is too small for the current subsampling rate, skipping video tick and destroying rendering context");
			renderingContext_.reset();
			return;
		}

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

void MainFilterContext::videoRender()
{
	obs_source_t *const parent = obs_filter_get_parent(source_);
	if (!parent) {
		logger_->debug("No parent source found, skipping video render");
		// Draw nothing to prevent unexpected background disclosure
		return;
	} else if (!obs_source_active(parent)) {
		logger_->debug("Parent source is not active, skipping video render");
		// Draw nothing to prevent unexpected background disclosure
		return;
	} else if (!obs_source_showing(parent)) {
		logger_->debug("Parent source is not showing, skipping video render");
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

std::shared_ptr<RenderingContext> MainFilterContext::createRenderingContext(std::uint32_t targetWidth,
									    std::uint32_t targetHeight)
{
	auto renderingContext = std::make_shared<RenderingContext>(source_, logger_, mainEffect_,
								   selfieSegmenterTaskQueue_, pluginConfig_,
								   pluginProperty_.subsamplingRate, targetWidth,
								   targetHeight, pluginProperty_.numThreads);

	renderingContext->applyPluginProperty(pluginProperty_);

	return renderingContext;
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
