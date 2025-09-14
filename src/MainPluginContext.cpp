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

#include <iostream>
#include <stdexcept>

#include <obs-module.h>

#include <obs-bridge-utils/ObsLogger.hpp>

using namespace kaito_tokyo::obs_bridge_utils;
using namespace kaito_tokyo::obs_backgroundremoval_lite;

namespace {

inline void ensureTexture(unique_gs_texture_t &texture, std::uint32_t width, std::uint32_t height,
			  gs_color_format format, std::uint32_t flags)
{
	if (!texture || gs_texture_get_width(texture.get()) != width ||
	    gs_texture_get_height(texture.get()) != height) {
		texture = make_unique_gs_texture(width, height, format, 1, NULL, flags);
	}
}

void ensureTextureReader(std::unique_ptr<AsyncTextureReader> &textureReader, std::uint32_t width, std::uint32_t height,
			 gs_color_format format)
{
	if (!textureReader || textureReader->getWidth() != width || textureReader->getHeight() != height) {
		textureReader = std::make_unique<AsyncTextureReader>(width, height, format);
	}
}

} // namespace

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

MainPluginContext::MainPluginContext(obs_data_t *_settings, obs_source_t *_source)
	: settings{_settings},
	  source{_source},
	  logger("[" PLUGIN_NAME "] "),
	  mainEffect(unique_obs_module_file("effects/main.effect"), logger),
	  selfieSegmenter(unique_obs_module_file("models/mediapipe_selfie_segmentation.ncnn.param"),
			  unique_obs_module_file("models/mediapipe_selfie_segmentation.ncnn.bin")),
	  selfieSegmenterTaskQueue(std::make_unique<TaskQueue>(logger)),
	  updateChecker(logger)
{
	futureLatestVersion = std::async(std::launch::async, [self = weak_from_this()]() -> std::optional<std::string> {
		if (auto s = self.lock()) {
			return s.get()->updateChecker.fetch();
		} else {
			blog(LOG_INFO, "MainPluginContext has been destroyed, skipping update check");
			return std::nullopt;
		}
	});
	update(settings);
}

MainPluginContext::~MainPluginContext() noexcept {}

void MainPluginContext::shutdown() noexcept
{
	selfieSegmenterTaskQueue.reset();
}

std::uint32_t MainPluginContext::getWidth() const noexcept
{
	return width;
}

std::uint32_t MainPluginContext::getHeight() const noexcept
{
	return height;
}

void MainPluginContext::getDefaults(obs_data_t *data)
{
	UNUSED_PARAMETER(data);
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

	return props;
}

void MainPluginContext::update(obs_data_t *_settings)
{
	settings = _settings;
}

void MainPluginContext::activate() {}

void MainPluginContext::deactivate() {}

void MainPluginContext::show() {}

void MainPluginContext::hide() {}

void MainPluginContext::videoTick(float seconds)
{
	UNUSED_PARAMETER(seconds);
}

void MainPluginContext::videoRender()
{
	if (width == 0 || height == 0) {
		logger.debug("Width or height is zero, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	ensureTextures();

	if (!bgrxSegmenterInput) {
		logger.error("bgrxSegmenterInput is null, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	if (!bgrxSegmenterInput) {
		logger.error("bgrxSegmenterInput is null, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	if (readerSegmenterInput) {
		try {
			readerSegmenterInput->sync();
		} catch (const std::exception &e) {
			logger.error("Failed to sync texture reader: {}", e.what());
		}
	}

	gs_texture_t *defaultRenderTarget = gs_get_render_target();
	gs_zstencil_t *defaultZStencil = gs_get_zstencil_target();
	gs_color_space defaultColorSpace = gs_get_color_space();

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_matrix_identity();

	gs_set_render_target_with_color_space(bgrxSourceInput.get(), nullptr, GS_CS_SRGB);

	if (!obs_source_process_filter_begin(source, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		logger.error("Could not begin processing filter");
		obs_source_skip_video_filter(source);
		return;
	}

	obs_source_process_filter_end(source, mainEffect.effect.get(), width, height);

	gs_set_render_target_with_color_space(bgrxSegmenterInput.get(), nullptr, GS_CS_SRGB);

	vec4 black = {0.0f, 0.0f, 0.0f, 1.0f};
	gs_clear(GS_CLEAR_COLOR, &black, 1.0f, 0);

	double scaleW = static_cast<double>(SelfieSegmenter::INPUT_WIDTH) / static_cast<double>(width);
	double scaleH = static_cast<double>(SelfieSegmenter::INPUT_HEIGHT) / static_cast<double>(height);
	double scale = std::min(scaleW, scaleH);

	std::uint32_t scaledW = static_cast<std::uint32_t>(std::round(width * scale));
	std::uint32_t scaledH = static_cast<std::uint32_t>(std::round(height * scale));

	std::uint32_t offsetX = (SelfieSegmenter::INPUT_WIDTH - scaledW) / 2;
	std::uint32_t offsetY = (SelfieSegmenter::INPUT_HEIGHT - scaledH) / 2;

	gs_set_viewport(offsetX, offsetY, scaledW, scaledH);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
	gs_matrix_identity();

	mainEffect.draw(width, height, bgrxSourceInput.get());

	gs_set_render_target_with_color_space(defaultRenderTarget, defaultZStencil, defaultColorSpace);

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();

	const auto &maskData = selfieSegmenter.getMask();

	const cv::Mat srcMask(SelfieSegmenter::INPUT_HEIGHT, SelfieSegmenter::INPUT_WIDTH, CV_8UC1,
			      const_cast<unsigned char *>(maskData.data()));

	const cv::Rect roi(offsetX, offsetY, scaledW, scaledH);
	const cv::Mat croppedMask = srcMask(roi);

	if (scaledMaskData.size() != static_cast<size_t>(width * height)) {
		scaledMaskData.resize(width * height);
	}

	cv::Mat dstMask(height, width, CV_8UC1, scaledMaskData.data());

	cv::resize(croppedMask, dstMask, dstMask.size(), 0, 0, cv::INTER_NEAREST);

	const std::uint8_t *r8Data = scaledMaskData.data();
	unique_gs_texture_t maskTexture = make_unique_gs_texture(width, height, GS_R8, 1, &r8Data, 0);
	mainEffect.drawWithMask(width, height, bgrxSourceInput.get(), maskTexture.get());

	if (readerSegmenterInput && bgrxSegmenterInput) {
		readerSegmenterInput->stage(bgrxSegmenterInput.get());
	}
}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
{
	if (width != frame->width || height != frame->height) {
		width = frame->width;
		height = frame->height;

		ensureTextures();
	}

	if (selfieSegmenterTaskQueue) {
		std::lock_guard<std::mutex> lock(selfieSegmenterPendingTaskTokenMutex);

		if (selfieSegmenterPendingTaskToken) {
			selfieSegmenterPendingTaskToken->store(true);
		}

		selfieSegmenterPendingTaskToken = selfieSegmenterTaskQueue->push([self = weak_from_this()](
											 const TaskQueue::CancellationToken
												 &token) {
			if (auto s = self.lock()) {
				if (!token->load()) {
					s.get()->selfieSegmenter.process(
						s.get()->readerSegmenterInput->getBuffer().data());
				} else {
					s.get()->getLogger().info(
						"Selfie segmentation task was cancelled, skipping processing");
				}
			} else {
				blog(LOG_INFO,
				     "MainPluginContext has been destroyed or task was cancelled, skipping processing");
			}
		});
	} else {
		logger.error("Task queue is not initialized");
	}

	return frame;
}

void MainPluginContext::ensureTextures()
{
	ensureTexture(bgrxSourceInput, width, height, GS_BGRX, GS_RENDER_TARGET);
	ensureTexture(bgrxSegmenterInput, SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT, GS_BGRX,
		      GS_RENDER_TARGET);
	ensureTextureReader(readerSegmenterInput, SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT, GS_BGRX);
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

const char *main_plugin_context_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("pluginName");
}

void *main_plugin_context_create(obs_data_t *settings, obs_source_t *source)
try {
	graphics_context_guard guard;
	auto self = std::make_shared<MainPluginContext>(settings, source);
	return new std::shared_ptr<MainPluginContext>(self);
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to create main plugin context: %s", e.what());
	return nullptr;
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to create main plugin context: unknown error");
	return nullptr;
}

void main_plugin_context_destroy(void *data)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_destroy called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->shutdown();
	delete self;

	graphics_context_guard guard;
	gs_unique::drain();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to destroy main plugin context: %s", e.what());

	graphics_context_guard guard;
	gs_unique::drain();
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to destroy main plugin context: unknown error");

	graphics_context_guard guard;
	gs_unique::drain();
}

std::uint32_t main_plugin_context_get_width(void *data)
{
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_get_width called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getWidth();
}

std::uint32_t main_plugin_context_get_height(void *data)
{
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_get_height called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getHeight();
}

void main_plugin_context_get_defaults(obs_data_t *data)
{
	MainPluginContext::getDefaults(data);
}

obs_properties_t *main_plugin_context_get_properties(void *data)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_get_properties called with null data");
		return obs_properties_create();
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getProperties();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to get properties: %s", e.what());
	return obs_properties_create();
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to get properties: unknown error");
	return obs_properties_create();
}

void main_plugin_context_update(void *data, obs_data_t *settings)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_update called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->update(settings);
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to update main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to update main plugin context: unknown error");
}

void main_plugin_context_activate(void *data)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_activate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->activate();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to activate main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to activate main plugin context: unknown error");
}

void main_plugin_context_deactivate(void *data)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_deactivate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->deactivate();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to deactivate main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to deactivate main plugin context: unknown error");
}

void main_plugin_context_show(void *data)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_show called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->show();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to show main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to show main plugin context: unknown error");
}

void main_plugin_context_hide(void *data)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_hide called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->hide();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to hide main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to hide main plugin context: unknown error");
}

void main_plugin_context_video_tick(void *data, float seconds)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_video_tick called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->videoTick(seconds);
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to tick main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to tick main plugin context: unknown error");
}

void main_plugin_context_video_render(void *data, gs_effect_t *_unused)
try {
	UNUSED_PARAMETER(_unused);

	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_video_render called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->videoRender();
	gs_unique::drain();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to render video in main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to render video in main plugin context: unknown error");
}

struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame)
try {
	if (!data) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] main_plugin_context_filter_video called with null data");
		return frame;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	obs_source_frame *result = self->get()->filterVideo(frame);
	gs_unique::drain();
	return result;
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to filter video in main plugin context: %s", e.what());
	return frame;
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to filter video in main plugin context: unknown error");
	return frame;
}

bool main_plugin_context_module_load()
try {
	return true;
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to load main plugin context: %s", e.what());
	return false;
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to load main plugin context: unknown error");
	return false;
}

void main_plugin_context_module_unload()
try {
	graphics_context_guard guard;
	gs_unique::drain();
} catch (const std::exception &e) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to unload main plugin context: %s", e.what());
} catch (...) {
	blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to unload main plugin context: unknown error");
}
