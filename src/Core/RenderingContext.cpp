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

#ifdef HAVE_COREML_SELFIE_SEGMENTER
#include <CoreMLSelfieSegmenter.hpp>
#endif

using namespace KaitoTokyo::Logger;
using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::TaskQueue;
using namespace KaitoTokyo::SelfieSegmenter;

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

namespace {

inline std::unique_ptr<ISelfieSegmenter> createSelfieSegmenter(const ILogger &logger, const PluginConfig &pluginConfig,
							       int computeUnit, int numThreads)
{
	if (computeUnit == ComputeUnit::kNull) {
		logger.info("Using null backend for selfie segmenter.");
		return std::make_unique<NullSelfieSegmenter>();
	} else if ((computeUnit & ComputeUnit::kCpuOnly) != 0) {
		logger.info("Using ncnn CPU backend for selfie segmenter.");
		return std::make_unique<NcnnSelfieSegmenter>(pluginConfig.selfieSegmenterParamPath.c_str(),
							     pluginConfig.selfieSegmenterBinPath.c_str(), numThreads);
	} else if ((computeUnit & ComputeUnit::kNcnnVulkanGpu) != 0) {
		int ncnnGpuIndex = computeUnit & ComputeUnit::kNcnnVulkanGpuIndexMask;
		logger.info("Using ncnn Vulkan GPU backend (GPU index: {}) for selfie segmenter.", ncnnGpuIndex);
		return std::make_unique<NcnnSelfieSegmenter>(pluginConfig.selfieSegmenterParamPath.c_str(),
							     pluginConfig.selfieSegmenterBinPath.c_str(), numThreads,
							     ncnnGpuIndex);
#ifdef HAVE_COREML_SELFIE_SEGMENTER
	} else if ((computeUnit & ComputeUnit::kCoreML) != 0) {
		logger.info("Using CoreML backend for selfie segmenter.");
		return std::make_unique<CoreMLSelfieSegmenter>();
#endif
	} else {
		throw std::runtime_error("Unsupported compute unit for selfie segmenter: " +
					 std::to_string(computeUnit));
	}
}

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

} // anonymous namespace

RenderingContext::RenderingContext(obs_source_t *const source, const ILogger &logger, const MainEffect &mainEffect,
				   ThrottledTaskQueue &selfieSegmenterTaskQueue, const PluginConfig &pluginConfig,
				   const std::uint32_t subsamplingRate, const std::uint32_t width,
				   const std::uint32_t height, const int computeUnit, const int numThreads)
	: source_(source),
	  logger_(logger),
	  mainEffect_(mainEffect),
	  selfieSegmenterTaskQueue_(selfieSegmenterTaskQueue),
	  pluginConfig_(pluginConfig),
	  subsamplingRate_(subsamplingRate),
	  computeUnit_(computeUnit),
	  numThreads_(numThreads),
	  selfieSegmenter_(createSelfieSegmenter(logger, pluginConfig, computeUnit, numThreads)),
	  region_{0, 0, width, height},
	  subRegion_{0, 0, (region_.width / subsamplingRate) & ~1u, (region_.height / subsamplingRate) & ~1u},
	  maskRoi_(getMaskRoiPosition(region_.width, region_.height, selfieSegmenter_)),
	  bgrxSource_(make_unique_gs_texture(region_.width, region_.height, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  r32fGrayscale_(make_unique_gs_texture(region_.width, region_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInput_(make_unique_gs_texture(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
						    static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX,
						    1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInputReader_(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
				   static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX),
	  segmenterInputBuffer_(selfieSegmenter_->getPixelCount() * 4),
	  r8SegmentationMask_(make_unique_gs_texture(maskRoi_.width, maskRoi_.height, GS_R8, 1, NULL, GS_DYNAMIC)),
	  r32fSubGFIntermediate_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r8SubGFGuide_(make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r8SubGFSource_(make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuide_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanSource_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSource_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSq_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFA_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFB_(
		  make_unique_gs_texture(subRegion_.width, subRegion_.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r8GuidedFilterResult_(make_unique_gs_texture(region_.width, region_.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
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

	doesNextVideoRenderReceiveNewFrame_ = true;
}

void RenderingContext::videoRender()
{
	FilterLevel filterLevel = filterLevel_;

	float guidedFilterEps = guidedFilterEps_;

	float maskGamma = maskGamma_;
	float maskLowerBound = maskLowerBound_;
	float maskUpperBoundMargin = maskUpperBoundMargin_;

	float timeAveragedFilteringAlpha = timeAveragedFilteringAlpha_;

	const bool doesNextVideoRenderReceiveNewFrame = doesNextVideoRenderReceiveNewFrame_.exchange(false);
	if (doesNextVideoRenderReceiveNewFrame) {
		if (filterLevel >= FilterLevel::Segmentation && !isStrictlySyncing_) {
			try {
				bgrxSegmenterInputReader_.sync();
			} catch (const std::exception &e) {
				logger_.error("Failed to sync texture reader: {}", e.what());
			}
		}

		mainEffect_.drawSource(bgrxSource_, source_);

		if (filterLevel >= FilterLevel::Segmentation) {
			constexpr vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

			mainEffect_.drawRoi(bgrxSegmenterInput_, bgrxSource_, &blackColor, maskRoi_.width, maskRoi_.height,
					   static_cast<float>(maskRoi_.x), static_cast<float>(maskRoi_.y));

			if (isStrictlySyncing_) {
				bgrxSegmenterInputReader_.stage(bgrxSegmenterInput_);
			}
		}

		if (filterLevel >= FilterLevel::GuidedFilter) {
			mainEffect_.convertToGrayscale(r32fGrayscale_, bgrxSource_);
		}

		if (filterLevel >= FilterLevel::Segmentation) {
			if (isStrictlySyncing_ && doesNextVideoRenderKickSelfieSegmentation_.exchange(false)) {
				bgrxSegmenterInputReader_.sync();
				selfieSegmenter_->process(bgrxSegmenterInputReader_.getBuffer().data());
			}

			const std::uint8_t *segmentationMaskData =
				selfieSegmenter_->getMask() + (maskRoi_.y * selfieSegmenter_->getWidth() + maskRoi_.x);
			gs_texture_set_image(r8SegmentationMask_.get(), segmentationMaskData,
					     static_cast<std::uint32_t>(selfieSegmenter_->getWidth()), 0);
		}

		if (filterLevel >= FilterLevel::GuidedFilter) {
			mainEffect_.resampleByNearestR8(r8SubGFGuide_, r32fGrayscale_);

			mainEffect_.resampleByNearestR8(r8SubGFSource_, r8SegmentationMask_);

			mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanGuide_, r8SubGFGuide_, r32fSubGFIntermediate_);
			mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanSource_, r8SubGFSource_, r32fSubGFIntermediate_);

			mainEffect_.applyBoxFilterWithMulR8KS17(r32fSubGFMeanGuideSource_, r8SubGFGuide_, r8SubGFSource_,
							       r32fSubGFIntermediate_);
			mainEffect_.applyBoxFilterWithSqR8KS17(r32fSubGFMeanGuideSq_, r8SubGFGuide_,
							      r32fSubGFIntermediate_);

			mainEffect_.calculateGuidedFilterAAndB(r32fSubGFA_, r32fSubGFB_, r32fSubGFMeanGuideSq_,
							      r32fSubGFMeanGuide_, r32fSubGFMeanGuideSource_,
							      r32fSubGFMeanSource_, guidedFilterEps);

			mainEffect_.finalizeGuidedFilter(r8GuidedFilterResult_, r32fGrayscale_, r32fSubGFA_, r32fSubGFB_);
		}

		if (filterLevel >= FilterLevel::TimeAveragedFilter) {
			std::size_t nextIndex = 1 - currentTimeAveragedMaskIndex_;
			mainEffect_.timeAveragedFiltering(r8TimeAveragedMasks_[nextIndex],
							 r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_],
							 r8GuidedFilterResult_, timeAveragedFilteringAlpha_);
			currentTimeAveragedMaskIndex_ = nextIndex;
		}
	}

	if (filterLevel == FilterLevel::Passthrough) {
		mainEffect_.directDraw(bgrxSource_);
	} else if (filterLevel == FilterLevel::Segmentation) {
		mainEffect_.directDrawWithMask(bgrxSource_, r8SegmentationMask_);
	} else if (filterLevel == FilterLevel::GuidedFilter) {
		mainEffect_.directDrawWithRefinedMask(bgrxSource_, r8GuidedFilterResult_, maskGamma_, maskLowerBound_,
						     maskUpperBoundMargin_);
	} else if (filterLevel == FilterLevel::TimeAveragedFilter) {
		mainEffect_.directDrawWithRefinedMask(bgrxSource_, r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_],
						     maskGamma_, maskLowerBound_, maskUpperBoundMargin_);
	} else {
		// Draw nothing to prevent unexpected background disclosure
	}

	if (filterLevel >= FilterLevel::Segmentation && doesNextVideoRenderReceiveNewFrame && !isStrictlySyncing_) {
		bgrxSegmenterInputReader_.stage(bgrxSegmenterInput_);
	}

	if (filterLevel >= FilterLevel::Segmentation && !isStrictlySyncing_ &&
	    doesNextVideoRenderKickSelfieSegmentation_.exchange(false)) {

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
	return frame;
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
