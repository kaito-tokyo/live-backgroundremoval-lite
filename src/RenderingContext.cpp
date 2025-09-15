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

#include <array>

#include "RenderingContext.hpp"
#include "SelfieSegmenter.hpp"

using namespace kaito_tokyo::obs_bridge_utils;

namespace {

struct TransformStateGuard {
	TransformStateGuard()
	{
		gs_viewport_push();
		gs_projection_push();
		gs_matrix_push();
	}
	~TransformStateGuard()
	{
		gs_matrix_pop();
		gs_projection_pop();
		gs_viewport_pop();
	}
};

struct RenderTargetGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	RenderTargetGuard()
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{
	}

	~RenderTargetGuard()
	{
		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};

std::array<std::uint32_t, 4> getMaskRoiDimension(std::uint32_t width, std::uint32_t height)
{
	using namespace kaito_tokyo::obs_backgroundremoval_lite;

	double widthScale = static_cast<double>(SelfieSegmenter::INPUT_WIDTH) / static_cast<double>(width);
	double heightScale = static_cast<double>(SelfieSegmenter::INPUT_HEIGHT) / static_cast<double>(height);
	double scale = std::min(widthScale, heightScale);

	std::uint32_t scaledWidth = static_cast<std::uint32_t>(std::round(width * scale));
	std::uint32_t scaledHeight = static_cast<std::uint32_t>(std::round(height * scale));

	std::uint32_t offsetX = (SelfieSegmenter::INPUT_WIDTH - scaledWidth) / 2;
	std::uint32_t offsetY = (SelfieSegmenter::INPUT_HEIGHT - scaledHeight) / 2;

	return {offsetX, offsetY, scaledWidth, scaledHeight};
}

} // anonymous namespace

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

RenderingContext::RenderingContext(obs_source_t *_source, const ILogger &_logger, const MainEffect &_mainEffect,
				   const ncnn::Net &_selfieSegmenterNet, std::uint32_t _width, std::uint32_t _height)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
	  readerSegmenterInput(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT, GS_BGRX),
	  selfieSegmenter(_selfieSegmenterNet),
	  width(_width),
	  height(_height),
	  bgrxOriginalImage(make_unique_gs_texture(width, height, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInput(make_unique_gs_texture(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT,
						    GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  maskRoiOffsetX(getMaskRoiDimension(width, height)[0]),
	  maskRoiOffsetY(getMaskRoiDimension(width, height)[1]),
	  maskRoiWidth(getMaskRoiDimension(width, height)[2]),
	  maskRoiHeight(getMaskRoiDimension(width, height)[3]),
	  r8SegmentationMask(make_unique_gs_texture(maskRoiWidth, maskRoiHeight, GS_R8, 1, NULL, GS_DYNAMIC))
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float seconds)
{
	UNUSED_PARAMETER(seconds);
}

void RenderingContext::renderOriginalImage()
{
	RenderTargetGuard renderTargetGuard;
	TransformStateGuard transformStateGuard;

	gs_set_render_target_with_color_space(bgrxOriginalImage.get(), nullptr, GS_CS_SRGB);

	if (!obs_source_process_filter_begin(source, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		logger.error("Could not begin processing filter");
		obs_source_skip_video_filter(source);
		return;
	}

	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_matrix_identity();

	obs_source_process_filter_end(source, mainEffect.effect.get(), width, height);
}

void RenderingContext::renderSegmenterInput()
{
	RenderTargetGuard renderTargetGuard;
	TransformStateGuard transformStateGuard;

	gs_set_render_target_with_color_space(bgrxSegmenterInput.get(), nullptr, GS_CS_SRGB);

	gs_clear(GS_CLEAR_COLOR, &blackColor, 1.0f, 0);

	gs_set_viewport(maskRoiOffsetX, maskRoiOffsetY, maskRoiWidth, maskRoiHeight);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
	gs_matrix_identity();

	mainEffect.draw(width, height, bgrxOriginalImage.get());
}

void RenderingContext::videoRender()
{
	try {
		readerSegmenterInput.sync();
	} catch (const std::exception &e) {
		logger.error("Failed to sync texture reader: {}", e.what());
	}

	renderOriginalImage();
	renderSegmenterInput();

	const std::uint8_t *segmentationMaskData =
		selfieSegmenter.getMask().data() + (maskRoiOffsetY * SelfieSegmenter::INPUT_WIDTH + maskRoiOffsetX);
	gs_texture_set_image(r8SegmentationMask.get(), segmentationMaskData, width, 0);
	mainEffect.drawWithMask(width, height, bgrxOriginalImage.get(), r8SegmentationMask.get());

	readerSegmenterInput.stage(bgrxSegmenterInput.get());
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	selfieSegmenter.process(readerSegmenterInput.getBuffer().data());
	return frame;
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
