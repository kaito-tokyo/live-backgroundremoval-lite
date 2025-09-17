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
				   std::uint32_t _width, std::uint32_t _height, const FilterLevel &_filterLevel,
				   const double &_gfEps, const double &_maskGamma, const double &_maskLowerBound,
				   const double &_maskUpperBound)
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
	  r16fOriginalGrayscale(make_unique_gs_texture(width, height, GS_R16F, 1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInput(make_unique_gs_texture(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT,
						    GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  maskRoiOffsetX(getMaskRoiDimension(width, height)[0]),
	  maskRoiOffsetY(getMaskRoiDimension(width, height)[1]),
	  maskRoiWidth(getMaskRoiDimension(width, height)[2]),
	  maskRoiHeight(getMaskRoiDimension(width, height)[3]),
	  r8SegmentationMask(make_unique_gs_texture(maskRoiWidth, maskRoiHeight, GS_R8, 1, NULL, GS_DYNAMIC)),
	  gfWidthSub(width / SUBSAMPLING_RATE),
	  gfHeightSub(height / SUBSAMPLING_RATE),
	  r8GFGuideSub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r8GFSourceSub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFMeanGuideSub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFMeanSourceSub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFMeanGuideSourceSub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFMeanGuideSqSub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFASub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFBSub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r8GFResult(make_unique_gs_texture(width, height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFTemporary1Sub(make_unique_gs_texture(gfWidthSub, gfHeightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  filterLevel(_filterLevel),
	  gfEps(_gfEps),
	  maskGamma(_maskGamma),
	  maskLowerBound(_maskLowerBound),
	  maskUpperBound(_maskUpperBound)

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

void RenderingContext::renderOriginalGrayscale(gs_texture_t *bgrxOriginalImage)
{
	mainEffect.convertToGrayscale(width, height, r16fOriginalGrayscale.get(), bgrxOriginalImage);
}

void RenderingContext::renderSegmenterInput(gs_texture_t *bgrxOriginalImage)
{
	RenderTargetGuard renderTargetGuard;
	TransformStateGuard transformStateGuard;

	gs_set_render_target_with_color_space(bgrxSegmenterInput.get(), nullptr, GS_CS_SRGB);

	gs_clear(GS_CLEAR_COLOR, &blackColor, 1.0f, 0);

	gs_set_viewport(maskRoiOffsetX, maskRoiOffsetY, maskRoiWidth, maskRoiHeight);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
	gs_matrix_identity();

	mainEffect.draw(width, height, bgrxOriginalImage);
}

void RenderingContext::renderSegmentationMask()
{
	const std::uint8_t *segmentationMaskData =
		selfieSegmenter.getMask().data() + (maskRoiOffsetY * SelfieSegmenter::INPUT_WIDTH + maskRoiOffsetX);
	gs_texture_set_image(r8SegmentationMask.get(), segmentationMaskData, SelfieSegmenter::INPUT_WIDTH, 0);
}

void RenderingContext::renderGuidedFilter(gs_texture_t *r16fOriginalGrayscale, gs_texture_t *r8SegmentationMask)
{
	mainEffect.resampleByNearestR8(gfWidthSub, gfHeightSub, r8GFGuideSub.get(), r16fOriginalGrayscale);

	mainEffect.resampleByNearestR8(gfWidthSub, gfHeightSub, r8GFSourceSub.get(), r8SegmentationMask);

	mainEffect.applyBoxFilterR8KS17(gfWidthSub, gfHeightSub, r32fGFMeanGuideSub.get(), r8GFGuideSub.get(),
					r32fGFTemporary1Sub.get());
	mainEffect.applyBoxFilterR8KS17(gfWidthSub, gfHeightSub, r32fGFMeanSourceSub.get(), r8GFSourceSub.get(),
					r32fGFTemporary1Sub.get());

	mainEffect.applyBoxFilterWithMulR8KS17(gfWidthSub, gfHeightSub, r32fGFMeanGuideSourceSub.get(),
					       r8GFGuideSub.get(), r8GFSourceSub.get(), r32fGFTemporary1Sub.get());
	mainEffect.applyBoxFilterWithSqR8KS17(gfWidthSub, gfHeightSub, r32fGFMeanGuideSqSub.get(), r8GFGuideSub.get(),
					      r32fGFTemporary1Sub.get());

	mainEffect.calculateGuidedFilterAAndB(gfWidthSub, gfHeightSub, r32fGFASub.get(), r32fGFBSub.get(),
					      r32fGFMeanGuideSqSub.get(), r32fGFMeanGuideSub.get(),
					      r32fGFMeanGuideSourceSub.get(), r32fGFMeanSourceSub.get(),
					      static_cast<float>(gfEps));

	mainEffect.finalizeGuidedFilter(width, height, r8GFResult.get(), r16fOriginalGrayscale, r32fGFASub.get(),
					r32fGFBSub.get());
}

void RenderingContext::kickSegmentationTask()
{
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

void RenderingContext::videoRender()
{
	FilterLevel actualFilterLevel = filterLevel == FilterLevel::Default ? FilterLevel::GuidedFilter : filterLevel;

	if (actualFilterLevel >= FilterLevel::Segmentation) {
		try {
			readerSegmenterInput.sync();
		} catch (const std::exception &e) {
			logger.error("Failed to sync texture reader: {}", e.what());
		}
	}

	renderOriginalImage();

	if (actualFilterLevel >= FilterLevel::GuidedFilter) {
		renderOriginalGrayscale(bgrxOriginalImage.get());
	}

	if (actualFilterLevel >= FilterLevel::Segmentation) {
		renderSegmenterInput(bgrxOriginalImage.get());
		renderSegmentationMask();
	}

	if (actualFilterLevel >= FilterLevel::GuidedFilter) {
		renderGuidedFilter(r16fOriginalGrayscale.get(), r8SegmentationMask.get());
	}

	if (actualFilterLevel == FilterLevel::Passthrough) {
		mainEffect.draw(width, height, bgrxOriginalImage.get());
	} else if (actualFilterLevel == FilterLevel::Segmentation) {
		mainEffect.drawWithMask(width, height, bgrxOriginalImage.get(), r8SegmentationMask.get());
	} else if (actualFilterLevel == FilterLevel::GuidedFilter) {
		mainEffect.drawWithRefinedMask(width, height, bgrxOriginalImage.get(), r8GFResult.get(), maskGamma,
					       maskLowerBound, maskUpperBound);
	} else {
		obs_source_skip_video_filter(source);
		return;
	}

	if (actualFilterLevel >= FilterLevel::Segmentation) {
		readerSegmenterInput.stage(bgrxSegmenterInput.get());
		kickSegmentationTask();
	}
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	logger.info("colormatrix {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}", frame->color_matrix[0], frame->color_matrix[1],
		    frame->color_matrix[2], frame->color_matrix[3], frame->color_matrix[4], frame->color_matrix[5],
		    frame->color_matrix[6], frame->color_matrix[7], frame->color_matrix[8], frame->color_matrix[9],
		    frame->color_matrix[10], frame->color_matrix[11], frame->color_matrix[12], frame->color_matrix[13],
		    frame->color_matrix[14], frame->color_matrix[15]);
	return frame;
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
