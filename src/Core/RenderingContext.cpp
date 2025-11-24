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

#include <NcnnSelfieSegmenter.hpp>

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::Logger;
using namespace KaitoTokyo::Memory;
using namespace KaitoTokyo::SelfieSegmenter;
using namespace KaitoTokyo::TaskQueue;

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

namespace {

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

BridgeUtils::unique_gs_texture_t RenderingContext::makeTexture(std::uint32_t width, std::uint32_t height,
							       enum gs_color_format color_format,
							       std::uint32_t flags) const noexcept
{
	unique_gs_texture_t texture = make_unique_gs_texture(width, height, color_format, 1, NULL, flags);
	if ((flags & GS_RENDER_TARGET) == GS_RENDER_TARGET) {
		TextureRenderGuard renderTargetGuard(texture);
		vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
		gs_clear(GS_CLEAR_COLOR, &clearColor, 0.0f, 0);
	} else if ((flags & GS_DYNAMIC) == GS_DYNAMIC) {
		// Dynamic textures should be initialized to zero
		std::vector<std::uint8_t> zeroData(
			static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
				static_cast<std::size_t>(AsyncTextureReader::getBytesPerPixel(color_format)),
			0);
		gs_texture_set_image(texture.get(), zeroData.data(),
				     width * AsyncTextureReader::getBytesPerPixel(color_format), 0);
	}
	return texture;
}

RenderingContextRegion RenderingContext::getMaskRoiPosition() const noexcept
{
	using namespace KaitoTokyo::LiveBackgroundRemovalLite;

	std::uint32_t selfieSegmenterWidth = static_cast<std::uint32_t>(selfieSegmenter_->getWidth());
	std::uint32_t selfieSegmenterHeight = static_cast<std::uint32_t>(selfieSegmenter_->getHeight());

	double widthScale = static_cast<double>(selfieSegmenterWidth) / static_cast<double>(region_.width);
	double heightScale = static_cast<double>(selfieSegmenterHeight) / static_cast<double>(region_.height);
	double scale = std::min(widthScale, heightScale);

	std::uint32_t scaledWidth = static_cast<std::uint32_t>(std::round(region_.width * scale));
	std::uint32_t scaledHeight = static_cast<std::uint32_t>(std::round(region_.height * scale));
	std::uint32_t offsetX = (selfieSegmenterWidth - scaledWidth) / 2;
	std::uint32_t offsetY = (selfieSegmenterHeight - scaledHeight) / 2;

	return {offsetX, offsetY, scaledWidth, scaledHeight};
}

std::vector<unique_gs_texture_t> RenderingContext::createReductionPyramid(std::uint32_t width,
									  std::uint32_t height) const
{
	std::vector<unique_gs_texture_t> pyramid;

	std::uint32_t currentWidth = width;
	std::uint32_t currentHeight = height;

	while (currentWidth > 1 || currentHeight > 1) {
		currentWidth = std::max(1u, (currentWidth + 1) / 2);
		currentHeight = std::max(1u, (currentHeight + 1) / 2);

		pyramid.push_back(makeTexture(currentWidth, currentHeight, GS_R32F, GS_RENDER_TARGET));
	}

	return pyramid;
}

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
	  selfieSegmenterMemoryBlockPool_(MemoryBlockPool::create(logger_, selfieSegmenter_->getPixelCount() * 4)),
	  region_{0, 0, width, height},
	  subRegion_{0, 0,
		     region_.width / subsamplingRate >= 2
			     ? (region_.width / subsamplingRate) & ~1u
			     : throw std::invalid_argument("Width too small for subsampling rate"),
		     region_.height / subsamplingRate >= 2
			     ? (region_.height / subsamplingRate) & ~1u
			     : throw std::invalid_argument("Height too small for subsampling rate")},
	  subPaddedRegion_{0, 0, bit_ceil(subRegion_.width), bit_ceil(subRegion_.height)},
	  maskRoi_(getMaskRoiPosition()),
	  bgrxSource_(makeTexture(region_.width, region_.height, GS_BGRX, GS_RENDER_TARGET)),
	  r32fLuma_(makeTexture(region_.width, region_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubLumas_{makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET),
			makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)},
	  r32fSubPaddedSquaredMotion_(
		  makeTexture(subPaddedRegion_.width, subPaddedRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fMeanSquaredMotionReductionPyramid_(
		  createReductionPyramid(subPaddedRegion_.width, subPaddedRegion_.height)),
	  r32fReducedMeanSquaredMotionReader_(1, 1, GS_R32F),
	  bgrxSegmenterInput_(makeTexture(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
					  static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX,
					  GS_RENDER_TARGET)),
	  bgrxSegmenterInputReader_(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
				    static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX),
	  segmenterInputBuffer_(selfieSegmenter_->getPixelCount() * 4),
	  r8SegmentationMask_(makeTexture(maskRoi_.width, maskRoi_.height, GS_R8, GS_DYNAMIC)),
	  r32fSubGFIntermediate_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubGFSource_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuide_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubGFMeanSource_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSource_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSq_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubGFA_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r32fSubGFB_(makeTexture(subRegion_.width, subRegion_.height, GS_R32F, GS_RENDER_TARGET)),
	  r8GuidedFilterResult_(makeTexture(region_.width, region_.height, GS_R8, GS_RENDER_TARGET)),
	  r8TimeAveragedMasks_{makeTexture(region_.width, region_.height, GS_R8, GS_RENDER_TARGET),
			       makeTexture(region_.width, region_.height, GS_R8, GS_RENDER_TARGET)}
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float seconds)
{
	timeSinceLastProcessFrame_ += seconds;
	if (timeSinceLastProcessFrame_ >= kMaximumIntervalSecondsBetweenProcessFrames_) {
		timeSinceLastProcessFrame_ -= kMaximumIntervalSecondsBetweenProcessFrames_;
		shouldNextVideoRenderProcessFrame_.store(true, std::memory_order_release);
	}
}

void RenderingContext::videoRender()
{
	const FilterLevel filterLevel = filterLevel_;

	const float motionIntensityThreshold = motionIntensityThreshold_.load(std::memory_order_acquire);

	const float guidedFilterEps = guidedFilterEps_.load(std::memory_order_acquire);

	const float maskGamma = maskGamma_.load(std::memory_order_acquire);
	const float maskLowerBound = maskLowerBound_.load(std::memory_order_acquire);
	const float maskUpperBoundMargin = maskUpperBoundMargin_.load(std::memory_order_acquire);

	const float timeAveragedFilteringAlpha = timeAveragedFilteringAlpha_.load(std::memory_order_acquire);

	const bool processingFrame = shouldNextVideoRenderProcessFrame_.exchange(false);

	if (processingFrame && filterLevel >= FilterLevel::Passthrough) {
		mainEffect_.drawSource(bgrxSource_, source_);
	}

	float motionIntensity = (filterLevel < FilterLevel::MotionIntensityThresholding) ? 1.0f : 0.0f;

	if (processingFrame && filterLevel >= FilterLevel::MotionIntensityThresholding) {
		mainEffect_.convertToLuma(r32fLuma_, bgrxSource_);

		const auto &lastSubLuma = r32fSubLumas_[currentSubLumaIndex_];
		const auto &currentSubLuma = r32fSubLumas_[1 - currentSubLumaIndex_];
		mainEffect_.resampleByNearestR8(currentSubLuma, r32fLuma_);

		mainEffect_.calculateSquaredMotion(r32fSubPaddedSquaredMotion_, currentSubLuma, lastSubLuma);

		currentSubLumaIndex_ = 1 - currentSubLumaIndex_;

		mainEffect_.reduce(r32fMeanSquaredMotionReductionPyramid_, r32fSubPaddedSquaredMotion_);

		r32fReducedMeanSquaredMotionReader_.stage(r32fMeanSquaredMotionReductionPyramid_.back());
	}

	if (processingFrame && filterLevel >= FilterLevel::Segmentation && hasNewSegmenterInput_) {
		hasNewSegmenterInput_ = false;
		try {
			bgrxSegmenterInputReader_.sync();
		} catch (const std::exception &e) {
			logger_.error("Failed to sync texture reader: {}", e.what());
		}
	}

	if (processingFrame && filterLevel >= FilterLevel::MotionIntensityThresholding) {
		r32fReducedMeanSquaredMotionReader_.sync();

		const float *meanSquaredMotionPtr =
			reinterpret_cast<const float *>(r32fReducedMeanSquaredMotionReader_.getBuffer().data());
		motionIntensity = *meanSquaredMotionPtr / (subRegion_.width * subRegion_.height);
	}

	const bool isCurrentMotionIntense = (motionIntensity >= motionIntensityThreshold);

	if (processingFrame && filterLevel >= FilterLevel::Segmentation && isCurrentMotionIntense) {
		constexpr vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

		mainEffect_.drawRoi(bgrxSegmenterInput_, bgrxSource_, &blackColor, maskRoi_.width, maskRoi_.height,
				    static_cast<float>(maskRoi_.x), static_cast<float>(maskRoi_.y));
	}

	if (processingFrame && hasNewSegmentationMask_.exchange(false) && filterLevel >= FilterLevel::Segmentation) {
		const std::uint8_t *segmentationMaskData =
			selfieSegmenter_->getMask() + (maskRoi_.y * selfieSegmenter_->getWidth() + maskRoi_.x);
		// gs_texture_set_image immediately uploads the data to GPU memory
		gs_texture_set_image(r8SegmentationMask_.get(), segmentationMaskData,
				     static_cast<std::uint32_t>(selfieSegmenter_->getWidth()), 0);
	}

	if (processingFrame && filterLevel >= FilterLevel::GuidedFilter) {
		const unique_gs_texture_t &currentSubLuma = r32fSubLumas_[currentSubLumaIndex_];
		mainEffect_.resampleByNearestR8(r32fSubGFSource_, r8SegmentationMask_);

		mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanGuide_, currentSubLuma, r32fSubGFIntermediate_);
		mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanSource_, r32fSubGFSource_, r32fSubGFIntermediate_);

		mainEffect_.applyBoxFilterWithMulR8KS17(r32fSubGFMeanGuideSource_, currentSubLuma, r32fSubGFSource_,
							r32fSubGFIntermediate_);
		mainEffect_.applyBoxFilterWithSqR8KS17(r32fSubGFMeanGuideSq_, currentSubLuma, r32fSubGFIntermediate_);

		mainEffect_.calculateGuidedFilterAAndB(r32fSubGFA_, r32fSubGFB_, r32fSubGFMeanGuideSq_,
						       r32fSubGFMeanGuide_, r32fSubGFMeanGuideSource_,
						       r32fSubGFMeanSource_, guidedFilterEps);

		mainEffect_.finalizeGuidedFilter(r8GuidedFilterResult_, r32fLuma_, r32fSubGFA_, r32fSubGFB_);
	}

	if (processingFrame && filterLevel >= FilterLevel::TimeAveragedFilter) {
		std::size_t nextIndex = 1 - currentTimeAveragedMaskIndex_;
		mainEffect_.timeAveragedFiltering(r8TimeAveragedMasks_[nextIndex],
						  r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_],
						  r8GuidedFilterResult_, timeAveragedFilteringAlpha);
		currentTimeAveragedMaskIndex_ = nextIndex;
	}

	if (filterLevel == FilterLevel::Passthrough) {
		mainEffect_.directDraw(bgrxSource_);
	} else if (filterLevel == FilterLevel::Segmentation ||
		   filterLevel == FilterLevel::MotionIntensityThresholding) {
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

	if (processingFrame && filterLevel >= FilterLevel::Segmentation && isCurrentMotionIntense) {
		bgrxSegmenterInputReader_.stage(bgrxSegmenterInput_);
		hasNewSegmenterInput_ = true;

		auto &bgrxSegmenterInputReaderBuffer = bgrxSegmenterInputReader_.getBuffer();
		auto segmenterInputBuffer = selfieSegmenterMemoryBlockPool_->acquire();
		if (!segmenterInputBuffer) {
			logger_.error("Failed to acquire memory block for segmenter input");
			return;
		}

		std::copy(bgrxSegmenterInputReaderBuffer.begin(), bgrxSegmenterInputReaderBuffer.end(),
			  segmenterInputBuffer->begin());

		selfieSegmenterTaskQueue_.push(
			[weakSelf = weak_from_this(), segmenterInputBuffer = std::move(segmenterInputBuffer)](
				const ThrottledTaskQueue::CancellationToken &token) {
				if (auto self = weakSelf.lock()) {
					if (token->load()) {
						return;
					}

					self->selfieSegmenter_->process(segmenterInputBuffer.get()->data());
					self->hasNewSegmentationMask_.store(true, std::memory_order_release);
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

void RenderingContext::applyPluginProperty(const PluginProperty &pluginProperty)
{
	FilterLevel newFilterLevel;
	if (pluginProperty.filterLevel == FilterLevel::Default) {
		newFilterLevel = FilterLevel::TimeAveragedFilter;
		logger_.info("Default filter level is parsed to be {}", static_cast<int>(newFilterLevel));
	} else {
		newFilterLevel = pluginProperty.filterLevel;
		logger_.info("Filter level set to {}", static_cast<int>(newFilterLevel));
	}
	filterLevel_.store(newFilterLevel, std::memory_order_release);

	float newMotionIntensityThreshold =
		static_cast<float>(std::pow(10.0, pluginProperty.motionIntensityThresholdPowDb / 10.0));
	motionIntensityThreshold_.store(newMotionIntensityThreshold, std::memory_order_release);
	logger_.info("Motion intensity threshold set to {}", newMotionIntensityThreshold);

	float newGuidedFilterEps = static_cast<float>(std::pow(10.0, pluginProperty.guidedFilterEpsPowDb / 10.0));
	guidedFilterEps_.store(newGuidedFilterEps, std::memory_order_release);
	logger_.info("Guided filter epsilon set to {}", newGuidedFilterEps);

	float newTimeAveragedFilteringAlpha = static_cast<float>(pluginProperty.timeAveragedFilteringAlpha);
	timeAveragedFilteringAlpha_.store(newTimeAveragedFilteringAlpha, std::memory_order_release);
	logger_.info("Time-averaged filtering alpha set to {}", newTimeAveragedFilteringAlpha);

	float newMaskGamma = static_cast<float>(pluginProperty.maskGamma);
	maskGamma_.store(newMaskGamma, std::memory_order_release);
	logger_.info("Mask gamma set to {}", newMaskGamma);

	float newMaskLowerBound = static_cast<float>(std::pow(10.0, pluginProperty.maskLowerBoundAmpDb / 20.0));
	maskLowerBound_.store(newMaskLowerBound, std::memory_order_release);
	logger_.info("Mask lower bound set to {}", newMaskLowerBound);

	float newMaskUpperBoundMargin =
		static_cast<float>(std::pow(10.0, pluginProperty.maskUpperBoundMarginAmpDb / 20.0));
	maskUpperBoundMargin_.store(newMaskUpperBoundMargin, std::memory_order_release);
	logger_.info("Mask upper bound margin set to {}", newMaskUpperBoundMargin);
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
