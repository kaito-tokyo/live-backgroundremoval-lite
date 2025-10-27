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

using namespace KaitoTokyo::BridgeUtils;
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

RenderingContext::RenderingContext(obs_source_t *const _source, const BridgeUtils::ILogger &_logger,
				   const MainEffect &_mainEffect,
				   BridgeUtils::ThrottledTaskQueue &_selfieSegmenterTaskQueue,
				   const PluginConfig &_pluginConfig, const std::uint32_t _subsamplingRate,
				   const std::uint32_t width, const std::uint32_t height, const int _computeUnit,
				   const int _numThreads)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
	  selfieSegmenterTaskQueue(_selfieSegmenterTaskQueue),
	  pluginConfig(_pluginConfig),
	  subsamplingRate(_subsamplingRate),
	  computeUnit(_computeUnit),
	  numThreads(_numThreads),
	  selfieSegmenter(createSelfieSegmenter(logger, pluginConfig, computeUnit, numThreads)),
	  region{0, 0, width, height},
	  subRegion{0, 0, (region.width / subsamplingRate) & ~1u, (region.height / subsamplingRate) & ~1u},
	  maskRoi(getMaskRoiPosition(region.width, region.height, selfieSegmenter)),
	  bgrxSource(make_unique_gs_texture(region.width, region.height, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  r32fGrayscale(make_unique_gs_texture(region.width, region.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInput(make_unique_gs_texture(static_cast<std::uint32_t>(selfieSegmenter->getWidth()),
						    static_cast<std::uint32_t>(selfieSegmenter->getHeight()), GS_BGRX,
						    1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInputReader(static_cast<std::uint32_t>(selfieSegmenter->getWidth()),
				   static_cast<std::uint32_t>(selfieSegmenter->getHeight()), GS_BGRX),
	  segmenterInputBuffer(selfieSegmenter->getPixelCount() * 4),
	  r8SegmentationMask(make_unique_gs_texture(maskRoi.width, maskRoi.height, GS_R8, 1, NULL, GS_DYNAMIC)),
	  r32fSubGFIntermediate(
		  make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r8SubGFGuide(make_unique_gs_texture(subRegion.width, subRegion.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r8SubGFSource(make_unique_gs_texture(subRegion.width, subRegion.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuide(
		  make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanSource(
		  make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSource(
		  make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFMeanGuideSq(
		  make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFA(make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r32fSubGFB(make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  r8GuidedFilterResult(make_unique_gs_texture(region.width, region.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r8TimeAveragedMasks{make_unique_gs_texture(region.width, region.height, GS_R8, 1, NULL, GS_RENDER_TARGET),
			      make_unique_gs_texture(region.width, region.height, GS_R8, 1, NULL, GS_RENDER_TARGET)}
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float seconds)
{
	FilterLevel _filterLevel = filterLevel;

	float _selfieSegmenterInterval = selfieSegmenterInterval;

	if (_filterLevel >= FilterLevel::Segmentation) {
		timeSinceLastSelfieSegmentation += seconds;

		if (timeSinceLastSelfieSegmentation >= _selfieSegmenterInterval) {
			timeSinceLastSelfieSegmentation -= _selfieSegmenterInterval;
			doesNextVideoRenderKickSelfieSegmentation = true;
		}
	}

	doesNextVideoRenderReceiveNewFrame = true;
}

void RenderingContext::videoRender()
{
	FilterLevel _filterLevel = filterLevel;

	float _guidedFilterEps = guidedFilterEps;

	float _maskGamma = maskGamma;
	float _maskLowerBound = maskLowerBound;
	float _maskUpperBoundMargin = maskUpperBoundMargin;

	float _timeAveragedFilteringAlpha = timeAveragedFilteringAlpha;

	const bool _doesNextVideoRenderReceiveNewFrame = doesNextVideoRenderReceiveNewFrame.exchange(false);
	if (_doesNextVideoRenderReceiveNewFrame) {
		if (_filterLevel >= FilterLevel::Segmentation && !isStrictlySyncing) {
			try {
				bgrxSegmenterInputReader.sync();
			} catch (const std::exception &e) {
				logger.error("Failed to sync texture reader: {}", e.what());
			}
		}

		mainEffect.drawSource(bgrxSource, source);

		if (_filterLevel >= FilterLevel::Segmentation) {
			constexpr vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

			mainEffect.drawRoi(bgrxSegmenterInput, bgrxSource, &blackColor, maskRoi.width, maskRoi.height,
					   static_cast<float>(maskRoi.x), static_cast<float>(maskRoi.y));

			if (isStrictlySyncing) {
				bgrxSegmenterInputReader.stage(bgrxSegmenterInput);
			}
		}

		if (_filterLevel >= FilterLevel::GuidedFilter) {
			mainEffect.convertToGrayscale(r32fGrayscale, bgrxSource);
		}

		if (_filterLevel >= FilterLevel::Segmentation) {
			if (isStrictlySyncing && doesNextVideoRenderKickSelfieSegmentation.exchange(false)) {
				bgrxSegmenterInputReader.sync();
				selfieSegmenter->process(bgrxSegmenterInputReader.getBuffer().data());
			}

			const std::uint8_t *segmentationMaskData =
				selfieSegmenter->getMask() + (maskRoi.y * selfieSegmenter->getWidth() + maskRoi.x);
			gs_texture_set_image(r8SegmentationMask.get(), segmentationMaskData,
					     static_cast<std::uint32_t>(selfieSegmenter->getWidth()), 0);
		}

		if (_filterLevel >= FilterLevel::GuidedFilter) {
			mainEffect.resampleByNearestR8(r8SubGFGuide, r32fGrayscale);

			mainEffect.resampleByNearestR8(r8SubGFSource, r8SegmentationMask);

			mainEffect.applyBoxFilterR8KS17(r32fSubGFMeanGuide, r8SubGFGuide, r32fSubGFIntermediate);
			mainEffect.applyBoxFilterR8KS17(r32fSubGFMeanSource, r8SubGFSource, r32fSubGFIntermediate);

			mainEffect.applyBoxFilterWithMulR8KS17(r32fSubGFMeanGuideSource, r8SubGFGuide, r8SubGFSource,
							       r32fSubGFIntermediate);
			mainEffect.applyBoxFilterWithSqR8KS17(r32fSubGFMeanGuideSq, r8SubGFGuide,
							      r32fSubGFIntermediate);

			mainEffect.calculateGuidedFilterAAndB(r32fSubGFA, r32fSubGFB, r32fSubGFMeanGuideSq,
							      r32fSubGFMeanGuide, r32fSubGFMeanGuideSource,
							      r32fSubGFMeanSource, _guidedFilterEps);

			mainEffect.finalizeGuidedFilter(r8GuidedFilterResult, r32fGrayscale, r32fSubGFA, r32fSubGFB);
		}

		if (_filterLevel >= FilterLevel::TimeAveragedFilter) {
			std::size_t nextIndex = 1 - currentTimeAveragedMaskIndex;
			mainEffect.timeAveragedFiltering(r8TimeAveragedMasks[nextIndex],
							 r8TimeAveragedMasks[currentTimeAveragedMaskIndex],
							 r8GuidedFilterResult, _timeAveragedFilteringAlpha);
			currentTimeAveragedMaskIndex = nextIndex;
		}
	}

	if (_filterLevel == FilterLevel::Passthrough) {
		mainEffect.directDraw(bgrxSource);
	} else if (_filterLevel == FilterLevel::Segmentation) {
		mainEffect.directDrawWithMask(bgrxSource, r8SegmentationMask);
	} else if (_filterLevel == FilterLevel::GuidedFilter) {
		mainEffect.directDrawWithRefinedMask(bgrxSource, r8GuidedFilterResult, _maskGamma, _maskLowerBound,
						     _maskUpperBoundMargin);
	} else if (_filterLevel == FilterLevel::TimeAveragedFilter) {
		mainEffect.directDrawWithRefinedMask(bgrxSource, r8TimeAveragedMasks[currentTimeAveragedMaskIndex],
						     _maskGamma, _maskLowerBound, _maskUpperBoundMargin);
	} else {
		// Draw nothing to prevent unexpected background disclosure
	}

	if (_filterLevel >= FilterLevel::Segmentation && _doesNextVideoRenderReceiveNewFrame && !isStrictlySyncing) {
		bgrxSegmenterInputReader.stage(bgrxSegmenterInput);
	}

	if (_filterLevel >= FilterLevel::Segmentation && !isStrictlySyncing &&
	    doesNextVideoRenderKickSelfieSegmentation.exchange(false)) {

		auto &bgrxSegmenterInputReaderBuffer = bgrxSegmenterInputReader.getBuffer();
		std::copy(bgrxSegmenterInputReaderBuffer.begin(), bgrxSegmenterInputReaderBuffer.end(),
			  segmenterInputBuffer.begin());
		selfieSegmenterTaskQueue.push(
			[weakSelf = weak_from_this()](const ThrottledTaskQueue::CancellationToken &token) {
				if (auto self = weakSelf.lock()) {
					if (token->load()) {
						return;
					}
					self->selfieSegmenter->process(self->segmenterInputBuffer.data());
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
