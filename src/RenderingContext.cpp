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
				   const ncnn::Net &_selfieSegmenterNet, ThrottledTaskQueue &_selfieSegmenterTaskQueue,
				   std::uint32_t _width, std::uint32_t _height)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
	  readerSegmenterInput(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT, GS_BGRX),
	  segmenterInputBuffer(SelfieSegmenter::PIXEL_COUNT * 4),
	  selfieSegmenter(_selfieSegmenterNet),
	  selfieSegmenterTaskQueue(_selfieSegmenterTaskQueue),
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
	gs_texture_set_image(r8SegmentationMask.get(), segmentationMaskData, SelfieSegmenter::INPUT_WIDTH, 0);

	int radius = 8;
	int kernelSize = radius * 2 + 1;
	int subsamplingRate = 4;
	// double eps = 0.02 * 0.02;
	// int radiusSub = radius / subsamplingRate;
	int widthSub = width / subsamplingRate;
	int heightSub = height / subsamplingRate;

	unique_gs_texture_t r8Grayscale =
		make_unique_gs_texture(width, height, GS_R8, 1, NULL, GS_RENDER_TARGET);
	unique_gs_texture_t r8GrayscaleSub =
		make_unique_gs_texture(widthSub, heightSub, GS_R8, 1, NULL, GS_RENDER_TARGET);
	unique_gs_texture_t r8MeanISub =
		make_unique_gs_texture(widthSub, heightSub, GS_R8, 1, NULL, GS_RENDER_TARGET);
	unique_gs_texture_t r8MeanPSub =
		make_unique_gs_texture(widthSub, heightSub, GS_R8, 1, NULL, GS_RENDER_TARGET);
	unique_gs_texture_t r8TemporarySub1 =
		make_unique_gs_texture(widthSub, heightSub, GS_R8, 1, NULL, GS_RENDER_TARGET);

	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_render_target_with_color_space(r8GrayscaleSub.get(), nullptr, GS_CS_SRGB);

		gs_set_viewport(0, 0, widthSub, heightSub);
		gs_ortho(0.0f, (float)widthSub, 0.0f, (float)heightSub, -100.0f, 100.0f);
		gs_matrix_identity();

		mainEffect.convertToGrayscale(widthSub, heightSub, bgrxOriginalImage.get());
	}

	mainEffect.applyBoxFilterR8(widthSub, heightSub, r8GrayscaleSub.get(), r8MeanISub.get(), kernelSize, r8TemporarySub1.get());

	mainEffect.applyBoxFilterR8(widthSub, heightSub, r8SegmentationMask.get(), r8MeanPSub.get(), kernelSize, r8TemporarySub1.get());

	mainEffect.drawWithMask(width, height, r8MeanISub.get(), r8MeanPSub.get());

	readerSegmenterInput.stage(bgrxSegmenterInput.get());

	auto &readerSegmenterInputBuffer = readerSegmenterInput.getBuffer();
	std::copy(readerSegmenterInputBuffer.begin(), readerSegmenterInputBuffer.end(), segmenterInputBuffer.begin());
	selfieSegmenterTaskQueue.push(
		[weakSelf = weak_from_this()](const ThrottledTaskQueue::CancellationToken &token) {
			if (auto self = weakSelf.lock()) {
				if (token->load()) {
					return;
				}
				self->selfieSegmenter.process(self->segmenterInputBuffer.data());
			} else {
				blog(LOG_INFO, "RenderingContext has been destroyed, skipping segmentation");
			}
		});
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	return frame;
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
