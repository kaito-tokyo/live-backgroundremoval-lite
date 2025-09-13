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

#include "plugin-support.h"
#include <obs-module.h>

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
	  mainEffect(unique_bfree_t(obs_module_file("effects/main.effect"))),
	  selfieSegmenter(unique_bfree_t(obs_module_file("models/mediapipe_selfie_segmentation.ncnn.param")),
			  unique_bfree_t(obs_module_file("models/mediapipe_selfie_segmentation.ncnn.bin")))
{
	update(settings);
}

MainPluginContext::~MainPluginContext() noexcept {}

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
	return obs_properties_create();
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
		obs_log(LOG_DEBUG, "Width or height is zero, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	ensureTextures();

	if (!bgrxSegmenterInput) {
		obs_log(LOG_ERROR, "bgrxSegmenterInput is null, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	if (!bgrxSegmenterInput) {
		obs_log(LOG_ERROR, "bgrxSegmenterInput is null, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	if (readerSegmenterInput) {
		try {
			readerSegmenterInput->sync();
		} catch (const std::exception &e) {
			obs_log(LOG_ERROR, "Failed to sync texture reader: %s", e.what());
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
		obs_log(LOG_ERROR, "Could not begin processing filter");
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

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(offsetX, offsetY, scaledW, scaledH);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
	gs_matrix_identity();

	mainEffect.draw(width, height, bgrxSourceInput.get());

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();

	gs_set_render_target_with_color_space(defaultRenderTarget, defaultZStencil, defaultColorSpace);

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();

	const auto &maskData = selfieSegmenter.getMask();

	std::vector<std::uint8_t> scaledMaskData(width * height);
	for (std::uint32_t y = 0; y < height; ++y) {
		for (std::uint32_t x = 0; x < width; ++x) {
			std::uint32_t src_x_in_crop =
				static_cast<std::uint32_t>(static_cast<double>(x) / width * scaledW);
			std::uint32_t src_y_in_crop =
				static_cast<std::uint32_t>(static_cast<double>(y) / height * scaledH);

			std::uint32_t src_x_in_mask = offsetX + src_x_in_crop;
			std::uint32_t src_y_in_mask = offsetY + src_y_in_crop;

			std::size_t src_idx = src_y_in_mask * SelfieSegmenter::INPUT_WIDTH + src_x_in_mask;
			std::size_t dst_idx = y * width + x;

			scaledMaskData[dst_idx] = maskData[src_idx];
		}
	}

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

	static int frameCount = 0;
	frameCount++;
	if (readerSegmenterInput) {
		selfieSegmenter.process(readerSegmenterInput->getBuffer().data());
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
	obs_log(LOG_ERROR, "Failed to create main plugin context: %s", e.what());
	return nullptr;
} catch (...) {
	obs_log(LOG_ERROR, "Failed to create main plugin context: unknown error");
	return nullptr;
}

void main_plugin_context_destroy(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_destroy called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	delete self;

	graphics_context_guard guard;
	gs_unique::drain();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to destroy main plugin context: %s", e.what());

	graphics_context_guard guard;
	gs_unique::drain();
} catch (...) {
	obs_log(LOG_ERROR, "Failed to destroy main plugin context: unknown error");

	graphics_context_guard guard;
	gs_unique::drain();
}

std::uint32_t main_plugin_context_get_width(void *data)
{
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_get_width called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getWidth();
}

std::uint32_t main_plugin_context_get_height(void *data)
{
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_get_height called with null data");
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
		obs_log(LOG_ERROR, "main_plugin_context_get_properties called with null data");
		return obs_properties_create();
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getProperties();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to get properties: %s", e.what());
	return obs_properties_create();
} catch (...) {
	obs_log(LOG_ERROR, "Failed to get properties: unknown error");
	return obs_properties_create();
}

void main_plugin_context_update(void *data, obs_data_t *settings)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_update called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->update(settings);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to update main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to update main plugin context: unknown error");
}

void main_plugin_context_activate(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_activate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->activate();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to activate main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to activate main plugin context: unknown error");
}

void main_plugin_context_deactivate(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_deactivate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->deactivate();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to deactivate main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to deactivate main plugin context: unknown error");
}

void main_plugin_context_show(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_show called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->show();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to show main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to show main plugin context: unknown error");
}

void main_plugin_context_hide(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_hide called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->hide();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to hide main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to hide main plugin context: unknown error");
}

void main_plugin_context_video_tick(void *data, float seconds)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_video_tick called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->videoTick(seconds);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to tick main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to tick main plugin context: unknown error");
}

void main_plugin_context_video_render(void *data, gs_effect_t *_unused)
try {
	UNUSED_PARAMETER(_unused);

	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_video_render called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->videoRender();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to render video in main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to render video in main plugin context: unknown error");
}

struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame)
try {
	if (!data) {
		obs_log(LOG_ERROR, "main_plugin_context_filter_video called with null data");
		return frame;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	obs_source_frame *result = self->get()->filterVideo(frame);
	gs_unique::drain();
	return result;
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to filter video in main plugin context: %s", e.what());
	return frame;
} catch (...) {
	obs_log(LOG_ERROR, "Failed to filter video in main plugin context: unknown error");
	return frame;
}

bool main_plugin_context_module_load()
try {
	return true;
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to load main plugin context: %s", e.what());
	return false;
} catch (...) {
	obs_log(LOG_ERROR, "Failed to load main plugin context: unknown error");
	return false;
}

void main_plugin_context_module_unload()
try {
	graphics_context_guard guard;
	gs_unique::drain();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to unload main plugin context: %s", e.what());
} catch (...) {
	obs_log(LOG_ERROR, "Failed to unload main plugin context: unknown error");
}
