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

#include "RenderingContext.hpp"

#include <array>

#include <NcnnSelfieSegmenter.hpp>
#include <NullSelfieSegmenter.hpp>

using namespace KaitoTokyo::Logger;
using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::TaskQueue;
using namespace KaitoTokyo::SelfieSegmenter;

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

namespace {

inline RenderingContextRegion getMaskRoiPosition(std::uint32_t width, std::uint32_t height,
						 const std::unique_ptr<ISelfieSegmenter> &selfieSegmenter)
{
	using namespace KaitoTokyo::LiveBackgroundRemovalLite;

	std::uint32_t selfieSegmenterWidth = static_cast<std::uint32_t>(selfieSegmenter->getWidth());
	std::uint32_t selfieSegmenterHeight = static_cast<std::uint32_t>(selfieSegmenter->getHeight());

	double widthScale = static_cast<double>(selfieSegmenterWidth) / static_cast<double>(width);
	double heightScale = static_cast<double>(selfieSegmenterHeight) / static_cast<double>(height);
	double scale = std::min(widthScale, heightScale);

	std::uint32_t scaledWidth = static_cast<std::uint32_t>(std::round(width * scale));
	std::uint32_t scaledHeight = static_cast<std::uint32_t>(std::round(height * scale));

	std::uint32_t offsetX = (selfieSegmenterWidth - scaledWidth) / 2;
	std::uint32_t offsetY = (selfieSegmenterHeight - scaledHeight) / 2;

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

		blog(LOG_INFO, "Creating reduction pyramid level: %ux%u", currentWidth, currentHeight);

		pyramid.push_back(
			make_unique_gs_texture(currentWidth, currentHeight, GS_R32F, 1, NULL, GS_RENDER_TARGET));
	}

	return pyramid;
}

inline std::uint32_t bit_ceil(std::uint32_t x)
{
	if (x == 0) {
		return 1;
	}
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

} // anonymous namespace

RenderingContext::RenderingContext(obs_source_t *const source, const ILogger &logger, const MainEffect &mainEffect,
				   ThrottledTaskQueue &selfieSegmenterTaskQueue, const PluginConfig &pluginConfig,
				   const std::uint32_t subsamplingRate, const std::uint32_t width,
				   const std::uint32_t height, const int numThreads)
	: source_(source),
	  logger_(logger),
	  mainEffect_(mainEffect),
	  selfieSegmenterTaskQueue_(selfieSegmenterTaskQueue),
	  pluginConfig_(pluginConfig),
	  subsamplingRate_(subsamplingRate),
	  numThreads_(numThreads),
	  selfieSegmenter_(std::make_unique<NcnnSelfieSegmenter>(pluginConfig.selfieSegmenterParamPath.c_str(),
								 pluginConfig.selfieSegmenterBinPath.c_str(),
								 numThreads)),
	  region_{0, 0, width, height},
	  subRegion_{0, 0, (region_.width / subsamplingRate) & ~1u, (region_.height / subsamplingRate) & ~1u},
	  subPaddedRegion_{0, 0, bit_ceil(subRegion_.width), bit_ceil(subRegion_.height)},
	  maskRoi_(getMaskRoiPosition(region_.width, region_.height, selfieSegmenter_)),
	  bgrxSource_(make_unique_gs_texture(region_.width, region_.height, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  r32fLuma_(make_unique_gs_texture(region_.width, region_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubLumas_{make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET),
			make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL,
					       GS_RENDER_TARGET)},
	  r32fSubPaddedSquaredMotion_(
		  make_unique_gs_texture(subPaddedRegion_.width, subPaddedRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fMeanSquaredMotionReductionPyramid_(
			  createReductionPyramid(subPaddedRegion_.width, subPaddedRegion_.height)),
	  r32fReducedMeanSquaredMotionReader_(1, 1, GS_R32F),
	  bgrxSegmenterInput_(make_unique_gs_texture(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
						     static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX,
						     1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInputReader_(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
				    static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX),
	  segmenterInputBuffer_(selfieSegmenter_->getPixelCount() * 4),
	  r8SegmentationMask_(make_unique_gs_texture(maskRoi_.width, maskRoi_.height, GS_R8, 1, NULL, GS_DYNAMIC)),
	  r32fSubGFIntermediate_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFSource_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuide_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanSource_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSource_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSq_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFA_(make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFB_(make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r8GuidedFilterResult_(
		  make_unique_gs_texture(region_.width, region_.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r8TimeAveragedMasks_{make_unique_gs_texture(region_.width, region_.height, GS_R8, 1, NULL, GS_RENDER_TARGET),
			       make_unique_gs_texture(region_.width, region_.height, GS_R8, 1, NULL, GS_RENDER_TARGET)}
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float seconds)
{
	FilterLevel filterLevel = filterLevel_;

	float selfieSegmenterInterval = selfieSegmenterInterval_;

	if (filterLevel >= FilterLevel::Segmentation) {
		timeSinceLastSelfieSegmentation_ += seconds;

		if (timeSinceLastSelfieSegmentation_ >= selfieSegmenterInterval) {
			timeSinceLastSelfieSegmentation_ -= selfieSegmenterInterval;
			doesNextVideoRenderKickSelfieSegmentation_ = true;
		}
	}

	timeSinceLastProcessFrame_ += seconds;
	if (timeSinceLastProcessFrame_ >= kMaximumIntervalSecondsBetweenProcessFrames_) {
		timeSinceLastProcessFrame_ -= kMaximumIntervalSecondsBetweenProcessFrames_;
		shouldNextVideoRenderProcessFrame_.store(true, std::memory_order_release);
	}
}

void RenderingContext::videoRender()
{
	FilterLevel filterLevel = filterLevel_;

	float guidedFilterEps = guidedFilterEps_;

	float maskGamma = maskGamma_;
	float maskLowerBound = maskLowerBound_;
	float maskUpperBoundMargin = maskUpperBoundMargin_;

	float timeAveragedFilteringAlpha = timeAveragedFilteringAlpha_;

	const bool shouldNextVideoRenderProcessFrame = shouldNextVideoRenderProcessFrame_.exchange(false);
	if (shouldNextVideoRenderProcessFrame) {
		if (filterLevel >= FilterLevel::Segmentation) {
			try {
				bgrxSegmenterInputReader_.sync();
			} catch (const std::exception &e) {
				logger_.error("Failed to sync texture reader: {}", e.what());
			}
		}

		mainEffect_.drawSource(bgrxSource_, source_);

		if (filterLevel >= FilterLevel::MotionIntensityThresholding) {
			mainEffect_.convertToLuma(r32fLuma_, bgrxSource_);

			const auto &lastSubLuma = r32fSubLumas_[currentSubLumaIndex_];
			const auto &currentSubLuma = r32fSubLumas_[1 - currentSubLumaIndex_];
			mainEffect_.resampleByNearestR8(currentSubLuma, r32fLuma_);


			mainEffect_.calculateSquaredMotion(r32fSubPaddedSquaredMotion_, currentSubLuma,
							   lastSubLuma);

			currentSubLumaIndex_ = 1 - currentSubLumaIndex_;

			mainEffect_.reduce(r32fMeanSquaredMotionReductionPyramid_, r32fSubPaddedSquaredMotion_);

			r32fReducedMeanSquaredMotionReader_.stage(r32fMeanSquaredMotionReductionPyramid_.back());
			r32fReducedMeanSquaredMotionReader_.sync();

			const float *meanSquaredMotionPtr =
				reinterpret_cast<const float *>(r32fReducedMeanSquaredMotionReader_.getBuffer().data());
			meanSquaredMotion_ = *meanSquaredMotionPtr / (subRegion_.width * subRegion_.height);
		}

		if (filterLevel >= FilterLevel::Segmentation) {
			constexpr vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

			mainEffect_.drawRoi(bgrxSegmenterInput_, bgrxSource_, &blackColor, maskRoi_.width,
					    maskRoi_.height, static_cast<float>(maskRoi_.x),
					    static_cast<float>(maskRoi_.y));
		}

		if (filterLevel >= FilterLevel::Segmentation) {
			const std::uint8_t *segmentationMaskData =
				selfieSegmenter_->getMask() + (maskRoi_.y * selfieSegmenter_->getWidth() + maskRoi_.x);
			gs_texture_set_image(r8SegmentationMask_.get(), segmentationMaskData,
					     static_cast<std::uint32_t>(selfieSegmenter_->getWidth()), 0);
		}

		if (filterLevel >= FilterLevel::GuidedFilter) {
			const unique_gs_texture_t &currentSubLuma = r32fSubLumas_[currentSubLumaIndex_];
			mainEffect_.resampleByNearestR8(r32fSubGFSource_, r8SegmentationMask_);

			mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanGuide_, currentSubLuma, r32fSubGFIntermediate_);
			mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanSource_, r32fSubGFSource_,
							 r32fSubGFIntermediate_);

			mainEffect_.applyBoxFilterWithMulR8KS17(r32fSubGFMeanGuideSource_, currentSubLuma,
								r32fSubGFSource_, r32fSubGFIntermediate_);
			mainEffect_.applyBoxFilterWithSqR8KS17(r32fSubGFMeanGuideSq_, currentSubLuma,
							       r32fSubGFIntermediate_);

			mainEffect_.calculateGuidedFilterAAndB(r32fSubGFA_, r32fSubGFB_, r32fSubGFMeanGuideSq_,
							       r32fSubGFMeanGuide_, r32fSubGFMeanGuideSource_,
							       r32fSubGFMeanSource_, guidedFilterEps);

			mainEffect_.finalizeGuidedFilter(r8GuidedFilterResult_, r32fLuma_, r32fSubGFA_, r32fSubGFB_);
		}

		if (filterLevel >= FilterLevel::TimeAveragedFilter) {
			std::size_t nextIndex = 1 - currentTimeAveragedMaskIndex_;
			mainEffect_.timeAveragedFiltering(r8TimeAveragedMasks_[nextIndex],
							  r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_],
							  r8GuidedFilterResult_, timeAveragedFilteringAlpha);
			currentTimeAveragedMaskIndex_ = nextIndex;
		}
	}

	if (filterLevel == FilterLevel::Passthrough) {
		mainEffect_.directDraw(bgrxSource_);
	} else if (filterLevel == FilterLevel::Segmentation || filterLevel == FilterLevel::MotionIntensityThresholding) {
		mainEffect_.directDrawWithMask(bgrxSource_, r8SegmentationMask_);
	} else if (filterLevel == FilterLevel::GuidedFilter) {
		mainEffect_.directDrawWithRefinedMask(bgrxSource_, r8GuidedFilterResult_, maskGamma, maskLowerBound,
						      maskUpperBoundMargin);
	} else if (filterLevel == FilterLevel::TimeAveragedFilter) {
		mainEffect_.directDrawWithRefinedMask(bgrxSource_, r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_],
						      maskGamma, maskLowerBound, maskUpperBoundMargin);
	} else {
		// Draw nothing to prevent unexpected background disclosure
	}

	if (filterLevel >= FilterLevel::Segmentation && shouldNextVideoRenderProcessFrame) {
		bgrxSegmenterInputReader_.stage(bgrxSegmenterInput_);
	}

	if (filterLevel >= FilterLevel::Segmentation && doesNextVideoRenderKickSelfieSegmentation_.exchange(false)) {

		auto &bgrxSegmenterInputReaderBuffer = bgrxSegmenterInputReader_.getBuffer();
		std::copy(bgrxSegmenterInputReaderBuffer.begin(), bgrxSegmenterInputReaderBuffer.end(),
			  segmenterInputBuffer_.begin());
		selfieSegmenterTaskQueue_.push(
			[weakSelf = weak_from_this()](const ThrottledTaskQueue::CancellationToken &token) {
				if (auto self = weakSelf.lock()) {
					if (token->load()) {
						return;
					}
					self->selfieSegmenter_->process(self->segmenterInputBuffer_.data());
				} else {
					blog(LOG_INFO, "RenderingContext has been destroyed, skipping segmentation");
				}
			});
	}
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	if (lastFrameTimestamp_ != frame->timestamp) {
		lastFrameTimestamp_ = frame->timestamp;
		shouldNextVideoRenderProcessFrame_.store(true, std::memory_order_release);
	}
	return frame;
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
