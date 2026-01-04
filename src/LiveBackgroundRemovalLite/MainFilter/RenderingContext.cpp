/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Background Removal Lite - MainFilter Module
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "RenderingContext.hpp"

#include <KaitoTokyo/SelfieSegmenter/BoundingBox.hpp>
#include <KaitoTokyo/SelfieSegmenter/NcnnSelfieSegmenter.hpp>

extern "C" const unsigned char mediapipe_selfie_segmentation_landscape_int8_ncnn_bin[];
extern "C" const unsigned int mediapipe_selfie_segmentation_landscape_int8_ncnn_bin_len;
extern "C" const char mediapipe_selfie_segmentation_landscape_int8_ncnn_param_text[];

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

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

ObsBridgeUtils::unique_gs_texture_t RenderingContext::makeTexture(std::uint32_t width, std::uint32_t height,
								  enum gs_color_format color_format,
								  std::uint32_t flags) const noexcept
{
	ObsBridgeUtils::unique_gs_texture_t texture =
		ObsBridgeUtils::make_unique_gs_texture(width, height, color_format, 1, NULL, flags);
	if ((flags & GS_RENDER_TARGET) == GS_RENDER_TARGET) {
		TextureRenderGuard renderTargetGuard(texture);
		vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
		gs_clear(GS_CLEAR_COLOR, &clearColor, 0.0f, 0);
	} else if ((flags & GS_DYNAMIC) == GS_DYNAMIC) {
		// Dynamic textures should be initialized to zero
		std::vector<std::uint8_t> zeroData(
			static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
				static_cast<std::size_t>(
					ObsBridgeUtils::AsyncTextureReader::getBytesPerPixel(color_format)),
			0);
		gs_texture_set_image(texture.get(), zeroData.data(),
				     width * ObsBridgeUtils::AsyncTextureReader::getBytesPerPixel(color_format), 0);
	}
	return texture;
}

RenderingContextRegion RenderingContext::getMaskRoiPosition() const noexcept
{
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

std::vector<ObsBridgeUtils::unique_gs_texture_t> RenderingContext::createReductionPyramid(std::uint32_t width,
											  std::uint32_t height) const
{
	std::vector<ObsBridgeUtils::unique_gs_texture_t> pyramid;

	std::uint32_t currentWidth = width;
	std::uint32_t currentHeight = height;

	while (currentWidth > 1 || currentHeight > 1) {
		currentWidth = std::max(1u, (currentWidth + 1) / 2);
		currentHeight = std::max(1u, (currentHeight + 1) / 2);

		pyramid.push_back(makeTexture(currentWidth, currentHeight, GS_R32F, GS_RENDER_TARGET));
	}

	return pyramid;
}

RenderingContext::RenderingContext(obs_source_t *const source, std::shared_ptr<const Logger::ILogger> logger,
				   const MainEffect &mainEffect,
				   TaskQueue::ThrottledTaskQueue &selfieSegmenterTaskQueue,
				   std::shared_ptr<Global::PluginConfig> pluginConfig,
				   const std::uint32_t subsamplingRate, const std::uint32_t width,
				   const std::uint32_t height, const int numThreads)
	: source_(source),
	  logger_(std::move(logger)),
	  mainEffect_(mainEffect),
	  selfieSegmenterTaskQueue_(selfieSegmenterTaskQueue),
	  pluginConfig_(pluginConfig),
	  subsamplingRate_(subsamplingRate),
	  numThreads_(numThreads),
	  selfieSegmenter_(std::make_unique<KaitoTokyo::SelfieSegmenter::NcnnSelfieSegmenter>(
		  mediapipe_selfie_segmentation_landscape_int8_ncnn_param_text,
		  static_cast<int>(mediapipe_selfie_segmentation_landscape_int8_ncnn_bin_len),
		  mediapipe_selfie_segmentation_landscape_int8_ncnn_bin, numThreads_)),
	  selfieSegmenterMemoryBlockPool_(
		  Memory::MemoryBlockPool::create(logger_, selfieSegmenter_->getPixelCount() * 4)),
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
	  segmenterRoi_(region_),
	  bgrxSegmenterInput_(makeTexture(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
					  static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX,
					  GS_RENDER_TARGET)),
	  bgrxSegmenterInputReader_(static_cast<std::uint32_t>(selfieSegmenter_->getWidth()),
				    static_cast<std::uint32_t>(selfieSegmenter_->getHeight()), GS_BGRX),
	  segmenterInputBuffer_(selfieSegmenter_->getPixelCount() * 4),
	  sourceRoi_(region_),
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

void RenderingContext::activate()
{
	shouldNextVideoRenderProcessFrame_.store(true, std::memory_order_release);
	shouldNextVideoRenderForceProcessFrame_.store(true, std::memory_order_release);
}

void RenderingContext::deactivate() {}

void RenderingContext::show()
{
	shouldNextVideoRenderProcessFrame_.store(true, std::memory_order_release);
	shouldNextVideoRenderForceProcessFrame_.store(true, std::memory_order_release);
}

void RenderingContext::hide() {}

void RenderingContext::videoTick(float)
{
	shouldNextVideoRenderProcessFrame_.store(true, std::memory_order_release);
}

void RenderingContext::videoRender()
{
	const FilterLevel filterLevel = filterLevel_.load(std::memory_order_relaxed);

	const float motionIntensityThreshold = motionIntensityThreshold_.load(std::memory_order_relaxed);

	const float guidedFilterEps = guidedFilterEps_.load(std::memory_order_relaxed);

	const float maskGamma = maskGamma_.load(std::memory_order_relaxed);
	const float maskLowerBound = maskLowerBound_.load(std::memory_order_relaxed);
	const float maskUpperBoundMargin = maskUpperBoundMargin_.load(std::memory_order_relaxed);

	const bool enableCenterFrame = enableCenterFrame_.load(std::memory_order_relaxed);

	const float timeAveragedFilteringAlpha = timeAveragedFilteringAlpha_.load(std::memory_order_relaxed);

	const bool processingFrame = shouldNextVideoRenderProcessFrame_.exchange(false, std::memory_order_acquire);
	const bool forceProcessingFrame =
		shouldNextVideoRenderForceProcessFrame_.exchange(false, std::memory_order_acquire);

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
			logger_->error("TextureSyncError", {{"message", e.what()}});
		}
	}

	if (processingFrame && filterLevel >= FilterLevel::MotionIntensityThresholding) {
		r32fReducedMeanSquaredMotionReader_.sync();

		const float *meanSquaredMotionPtr =
			reinterpret_cast<const float *>(r32fReducedMeanSquaredMotionReader_.getBuffer().data());
		motionIntensity = *meanSquaredMotionPtr / (subRegion_.width * subRegion_.height);
	}

	const bool isCurrentMotionIntense = (motionIntensity >= motionIntensityThreshold);

	if (processingFrame && filterLevel >= FilterLevel::Segmentation &&
	    (isCurrentMotionIntense || forceProcessingFrame)) {
		constexpr vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

		const double targetW = static_cast<double>(selfieSegmenter_->getWidth());
		const double targetH = static_cast<double>(selfieSegmenter_->getHeight());

		double fitScaleX = targetW / static_cast<double>(segmenterRoi_.width);
		double fitScaleY = targetH / static_cast<double>(segmenterRoi_.height);
		double scale = std::min(fitScaleX, fitScaleY);

		std::uint32_t width = static_cast<std::uint32_t>(std::round(region_.width * scale));
		std::uint32_t height = static_cast<std::uint32_t>(std::round(region_.height * scale));

		double roiCenterX = segmenterRoi_.x + segmenterRoi_.width / 2.0;
		double roiCenterY = segmenterRoi_.y + segmenterRoi_.height / 2.0;

		float x = static_cast<float>((targetW / 2.0) - (roiCenterX * scale));
		float y = static_cast<float>((targetH / 2.0) - (roiCenterY * scale));

		mainEffect_.drawRoi(bgrxSegmenterInput_, bgrxSource_, &blackColor, width, height, x, y);
	}

	if (processingFrame && filterLevel >= FilterLevel::Segmentation) {
		if (hasNewSegmentationMask_.load(std::memory_order_relaxed) &&
		    hasNewSegmentationMask_.exchange(false, std::memory_order_acquire)) {
			if (enableCenterFrame) {
				SelfieSegmenter::BoundingBox bb;
				bb.calculateBoundingBoxFrom256x144(selfieSegmenter_->getMask(), 200);

				const std::uint64_t bbX = static_cast<std::uint64_t>(bb.x);
				const std::uint64_t bbY = static_cast<std::uint64_t>(bb.y);
				const std::uint64_t bbW = static_cast<std::uint64_t>(bb.width);
				const std::uint64_t bbH = static_cast<std::uint64_t>(bb.height);

				const std::uint64_t roiX = static_cast<std::uint64_t>(segmenterRoi_.x);
				const std::uint64_t roiY = static_cast<std::uint64_t>(segmenterRoi_.y);
				const std::uint64_t roiW = static_cast<std::uint64_t>(segmenterRoi_.width);
				const std::uint64_t roiH = static_cast<std::uint64_t>(segmenterRoi_.height);

				const std::uint64_t baseW = static_cast<std::uint64_t>(selfieSegmenter_->getWidth());
				const std::uint64_t baseH = static_cast<std::uint64_t>(selfieSegmenter_->getHeight());

				if (baseW > 0 && baseH > 0) {
					sourceRoi_.x =
						static_cast<std::uint32_t>(((bbX * roiW) + (baseW / 2)) / baseW + roiX);

					sourceRoi_.y =
						static_cast<std::uint32_t>(((bbY * roiH) + (baseH / 2)) / baseH + roiY);

					sourceRoi_.width =
						static_cast<std::uint32_t>(((bbW * roiW) + (baseW / 2)) / baseW);

					sourceRoi_.height =
						static_cast<std::uint32_t>(((bbH * roiH) + (baseH / 2)) / baseH);
				}
			}

			const std::uint8_t *segmentationMaskData =
				selfieSegmenter_->getMask() + (maskRoi_.y * selfieSegmenter_->getWidth() + maskRoi_.x);

			// gs_texture_set_image immediately uploads the data to GPU memory
			gs_texture_set_image(r8SegmentationMask_.get(), segmentationMaskData,
					     static_cast<std::uint32_t>(selfieSegmenter_->getWidth()), 0);
		}
	}

	if (processingFrame && filterLevel >= FilterLevel::GuidedFilter) {
		const ObsBridgeUtils::unique_gs_texture_t &currentSubLuma = r32fSubLumas_[currentSubLumaIndex_];
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

	if (enableCenterFrame) {
		gs_matrix_push();

		float scaleFactor;
		if (sourceRoi_.width > 0 && sourceRoi_.height > 0) {
			float widthScale = static_cast<float>(region_.width) / static_cast<float>(sourceRoi_.width);
			float heightScale = static_cast<float>(region_.height) / static_cast<float>(sourceRoi_.height);
			scaleFactor = std::min(widthScale, heightScale);
		} else {
			scaleFactor = 1.0f;
		}

		float displayW = static_cast<float>(sourceRoi_.width) * scaleFactor;
		float displayH = static_cast<float>(sourceRoi_.height) * scaleFactor;

		float offsetX = (static_cast<float>(region_.width) - displayW) / 2.0f;
		float offsetY = (static_cast<float>(region_.height) - displayH);

		vec3 vecDest{offsetX, offsetY, 0.0f};
		gs_matrix_translate(&vecDest);

		vec3 vecScale{scaleFactor, scaleFactor, 1.0f};
		gs_matrix_scale(&vecScale);

		vec3 vecSource{-static_cast<float>(sourceRoi_.x), -static_cast<float>(sourceRoi_.y), 0.0f};
		gs_matrix_translate(&vecSource);
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

	if (enableCenterFrame) {
		gs_matrix_pop();
	}

	if (processingFrame && filterLevel >= FilterLevel::Segmentation &&
	    (isCurrentMotionIntense || forceProcessingFrame)) {
		bgrxSegmenterInputReader_.stage(bgrxSegmenterInput_);
		hasNewSegmenterInput_ = true;

		auto &bgrxSegmenterInputReaderBuffer = bgrxSegmenterInputReader_.getBuffer();
		auto segmenterInputBuffer = selfieSegmenterMemoryBlockPool_->acquire();
		if (!segmenterInputBuffer) {
			logger_->error("MemoryBlockAcquisitionError");
			return;
		}

		std::copy(bgrxSegmenterInputReaderBuffer.begin(), bgrxSegmenterInputReaderBuffer.end(),
			  segmenterInputBuffer->begin());

		selfieSegmenterTaskQueue_.push(
			[weakSelf = weak_from_this(), segmenterInputBuffer = std::move(segmenterInputBuffer)](
				const TaskQueue::ThrottledTaskQueue::CancellationToken &token) {
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

void RenderingContext::applyPluginProperty(const PluginProperty &pluginProperty)
{
	FilterLevel newFilterLevel = (pluginProperty.filterLevel == FilterLevel::Default)
					     ? FilterLevel::TimeAveragedFilter
					     : pluginProperty.filterLevel;

	float newMotionIntensityThreshold =
		static_cast<float>(std::pow(10.0, pluginProperty.motionIntensityThresholdPowDb / 10.0));

	float newGuidedFilterEps = static_cast<float>(std::pow(10.0, pluginProperty.guidedFilterEpsPowDb / 10.0));

	float newTimeAveragedFilteringAlpha = static_cast<float>(pluginProperty.timeAveragedFilteringAlpha);

	float newMaskGamma = static_cast<float>(pluginProperty.maskGamma);

	float newMaskLowerBound = static_cast<float>(std::pow(10.0, pluginProperty.maskLowerBoundAmpDb / 20.0));

	float newMaskUpperBoundMargin =
		static_cast<float>(std::pow(10.0, pluginProperty.maskUpperBoundMarginAmpDb / 20.0));

	filterLevel_.store(newFilterLevel, std::memory_order_relaxed);
	motionIntensityThreshold_.store(newMotionIntensityThreshold, std::memory_order_relaxed);
	guidedFilterEps_.store(newGuidedFilterEps, std::memory_order_relaxed);
	timeAveragedFilteringAlpha_.store(newTimeAveragedFilteringAlpha, std::memory_order_relaxed);
	maskGamma_.store(newMaskGamma, std::memory_order_relaxed);
	maskLowerBound_.store(newMaskLowerBound, std::memory_order_relaxed);
	maskUpperBoundMargin_.store(newMaskUpperBoundMargin, std::memory_order_relaxed);
	enableCenterFrame_.store(pluginProperty.enableCenterFrame, std::memory_order_relaxed);

	logger_->info("PluginPropertySet",
		      {{"key", "filterLevel"}, {"value", std::to_string(static_cast<int>(newFilterLevel))}});
	logger_->info("PluginPropertySet",
		      {{"key", "motionIntensityThreshold"}, {"value", std::to_string(newMotionIntensityThreshold)}});
	logger_->info("PluginPropertySet", {{"key", "guidedFilterEps"}, {"value", std::to_string(newGuidedFilterEps)}});
	logger_->info("PluginPropertySet", {{"key", "timeAveragedFilteringAlpha"},
					    {"value", std::to_string(newTimeAveragedFilteringAlpha)}});
	logger_->info("PluginPropertySet", {{"key", "maskGamma"}, {"value", std::to_string(newMaskGamma)}});
	logger_->info("PluginPropertySet", {{"key", "maskLowerBound"}, {"value", std::to_string(newMaskLowerBound)}});
	logger_->info("PluginPropertySet",
		      {{"key", "maskUpperBoundMargin"}, {"value", std::to_string(newMaskUpperBoundMargin)}});
	logger_->info("PluginPropertySet",
		      {{"key", "enableCenterFrame"}, {"value", pluginProperty.enableCenterFrame ? "true" : "false"}});
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
