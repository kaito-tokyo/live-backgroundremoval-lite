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

#include <array>

#include "RenderingContext.hpp"
#include "../BridgeUtils/AsyncTextureReader.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace {

std::array<std::uint32_t, 4> getMaskRoiDimension(std::uint32_t width, std::uint32_t height)
{
	using namespace KaitoTokyo::LiveBackgroundRemovalLite;

	double widthScale = static_cast<double>(SelfieSegmenter::INPUT_WIDTH) / static_cast<double>(width);
	double heightScale = static_cast<double>(SelfieSegmenter::INPUT_HEIGHT) / static_cast<double>(height);
	double scale = std::min(widthScale, heightScale);

	std::uint32_t scaledWidth = static_cast<std::uint32_t>(std::round(width * scale));
	std::uint32_t scaledHeight = static_cast<std::uint32_t>(std::round(height * scale));

	std::uint32_t offsetX = (SelfieSegmenter::INPUT_WIDTH - scaledWidth) / 2;
	std::uint32_t offsetY = (SelfieSegmenter::INPUT_HEIGHT - scaledHeight) / 2;

	return {offsetX, offsetY, scaledWidth, scaledHeight};
}

inline std::vector<unique_gs_texture_t> createReductionPyramid(std::uint32_t width, std::uint32_t height)
{
	std::vector<unique_gs_texture_t> pyramid;

	std::uint32_t currentWidth = width;
	std::uint32_t currentHeight = height;

	while (currentWidth > 1 || currentHeight > 1) {
		currentWidth = std::max(1u, (currentWidth + 1) / 2);
		currentHeight = std::max(1u, (currentHeight + 1) / 2);

		pyramid.push_back(
			make_unique_gs_texture(currentWidth, currentHeight, GS_R32F, 1, NULL, GS_RENDER_TARGET));
	}

	return pyramid;
}

} // anonymous namespace

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

RenderingContext::RenderingContext(obs_source_t *_source, const ILogger &_logger, const MainEffect &_mainEffect,
				   const ncnn::Net &_selfieSegmenterNet, ThrottledTaskQueue &_selfieSegmenterTaskQueue,
				   PluginConfig _pluginConfig, std::uint32_t _subsamplingRate, std::uint32_t _width,
				   std::uint32_t _height)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
	  readerSegmenterInput(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT, GS_BGRX),
	  selfieSegmenter(_selfieSegmenterNet),
	  selfieSegmenterTaskQueue(_selfieSegmenterTaskQueue),
	  pluginConfig(_pluginConfig),
	  subsamplingRate(_subsamplingRate),
	  width(_width),
	  height(_height),
	  widthSub((width / subsamplingRate) & ~1u),
	  heightSub((height / subsamplingRate) & ~1u),
	  bgrxOriginalImage(make_unique_gs_texture(width, height, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  r32fOriginalGrayscale(make_unique_gs_texture(width, height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInput(make_unique_gs_texture(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT,
						    GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  segmenterInputBuffer(SelfieSegmenter::PIXEL_COUNT * 4),
	  maskRoiOffsetX(getMaskRoiDimension(width, height)[0]),
	  maskRoiOffsetY(getMaskRoiDimension(width, height)[1]),
	  maskRoiWidth(getMaskRoiDimension(width, height)[2]),
	  maskRoiHeight(getMaskRoiDimension(width, height)[3]),
	  r8SegmentationMask(make_unique_gs_texture(maskRoiWidth, maskRoiHeight, GS_R8, 1, NULL, GS_DYNAMIC)),
	  r8SubGFGuide(make_unique_gs_texture(widthSub, heightSub, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r8SubGFSource(make_unique_gs_texture(widthSub, heightSub, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuide(make_unique_gs_texture(widthSub, heightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanSource(make_unique_gs_texture(widthSub, heightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSource(make_unique_gs_texture(widthSub, heightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSq(make_unique_gs_texture(widthSub, heightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFA(make_unique_gs_texture(widthSub, heightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFB(make_unique_gs_texture(widthSub, heightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r8GFResult(make_unique_gs_texture(width, height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r32fGFTemporary1Sub(make_unique_gs_texture(widthSub, heightSub, GS_R32F, 1, NULL, GS_RENDER_TARGET))
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float seconds)
{
	FilterLevel _filterLevel = filterLevel;

	float _selfieSegmenterFps = selfieSegmenterFps;

	if (_filterLevel >= FilterLevel::Segmentation) {
		timeSinceLastSelfieSegmentation += seconds;
		const float interval = 1.0f / _selfieSegmenterFps;

		if (timeSinceLastSelfieSegmentation >= interval) {
			timeSinceLastSelfieSegmentation -= interval;
			kickSegmentationTask();
		}
	}

	doesNextVideoRenderReceiveNewFrame = true;
}

void RenderingContext::renderOriginalImage()
{
	RenderTargetGuard renderTargetGuard;
	TransformStateGuard transformStateGuard;

	gs_set_render_target_with_color_space(bgrxOriginalImage.get(), nullptr, GS_CS_SRGB);

	if (!obs_source_process_filter_begin(source, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		logger.error("Could not begin processing filter");
		return;
	}

	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_matrix_identity();

	obs_source_process_filter_end(source, mainEffect.effect.get(), width, height);
}

void RenderingContext::renderOriginalGrayscale(gs_texture_t *bgrxOriginalImage)
{
	mainEffect.convertToGrayscale(width, height, r32fOriginalGrayscale.get(), bgrxOriginalImage);
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

void RenderingContext::renderGuidedFilter(gs_texture_t *r16fOriginalGrayscale, gs_texture_t *r8SegmentationMask,
					  float gfEps)
{
	mainEffect.resampleByNearestR8(widthSub, heightSub, r8SubGFGuide.get(), r16fOriginalGrayscale);

	mainEffect.resampleByNearestR8(widthSub, heightSub, r8SubGFSource.get(), r8SegmentationMask);

	mainEffect.applyBoxFilterR8KS17(widthSub, heightSub, r32fSubGFMeanGuide.get(), r8SubGFGuide.get(),
					r32fGFTemporary1Sub.get());
	mainEffect.applyBoxFilterR8KS17(widthSub, heightSub, r32fSubGFMeanSource.get(), r8SubGFSource.get(),
					r32fGFTemporary1Sub.get());

	mainEffect.applyBoxFilterWithMulR8KS17(widthSub, heightSub, r32fSubGFMeanGuideSource.get(), r8SubGFGuide.get(),
					       r8SubGFSource.get(), r32fGFTemporary1Sub.get());
	mainEffect.applyBoxFilterWithSqR8KS17(widthSub, heightSub, r32fSubGFMeanGuideSq.get(), r8SubGFGuide.get(),
					      r32fGFTemporary1Sub.get());

	mainEffect.calculateGuidedFilterAAndB(widthSub, heightSub, r32fSubGFA.get(), r32fSubGFB.get(),
					      r32fSubGFMeanGuideSq.get(), r32fSubGFMeanGuide.get(),
					      r32fSubGFMeanGuideSource.get(), r32fSubGFMeanSource.get(), gfEps);

	mainEffect.finalizeGuidedFilter(width, height, r8GFResult.get(), r16fOriginalGrayscale, r32fSubGFA.get(),
					r32fSubGFB.get());
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
	FilterLevel _filterLevel = filterLevel;

	float _gfEps = gfEps;

	float _maskGamma = maskGamma;
	float _maskLowerBound = maskLowerBound;
	float _maskUpperBoundMargin = maskUpperBoundMargin;

	const bool needNewFrame = doesNextVideoRenderReceiveNewFrame;
	if (needNewFrame) {
		doesNextVideoRenderReceiveNewFrame = false;

		if (_filterLevel >= FilterLevel::Segmentation) {
			try {
				readerSegmenterInput.sync();
			} catch (const std::exception &e) {
				logger.error("Failed to sync texture reader: {}", e.what());
			}
		}

		renderOriginalImage();

		if (_filterLevel >= FilterLevel::GuidedFilter) {
			renderOriginalGrayscale(bgrxOriginalImage.get());
		}

		if (_filterLevel >= FilterLevel::Segmentation) {
			renderSegmenterInput(bgrxOriginalImage.get());
			renderSegmentationMask();
		}

		if (_filterLevel >= FilterLevel::GuidedFilter) {
			renderGuidedFilter(r32fOriginalGrayscale.get(), r8SegmentationMask.get(), _gfEps);
		}
	}

	if (_filterLevel == FilterLevel::Passthrough) {
		mainEffect.draw(width, height, bgrxOriginalImage.get());
	} else if (_filterLevel == FilterLevel::Segmentation) {
		mainEffect.drawWithMask(width, height, bgrxOriginalImage.get(), r8SegmentationMask.get());
	} else if (_filterLevel == FilterLevel::GuidedFilter) {
		mainEffect.drawWithRefinedMask(width, height, bgrxOriginalImage.get(), r8GFResult.get(), _maskGamma,
					       _maskLowerBound, _maskUpperBoundMargin);
	} else {
		// Draw nothing to prevent unexpected background disclosure
	}

	if (needNewFrame && _filterLevel >= FilterLevel::Segmentation) {
		readerSegmenterInput.stage(bgrxSegmenterInput);
	}
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	return frame;
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
