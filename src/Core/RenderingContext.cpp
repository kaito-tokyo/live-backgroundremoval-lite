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

#include "../BridgeUtils/AsyncTextureReader.hpp"

#include "../SelfieSegmenter/NcnnSelfieSegmenter.hpp"
#include "../SelfieSegmenter/NullSelfieSegmenter.hpp"

#ifdef HAVE_COREML_SELFIE_SEGMENTER
#include "../SelfieSegmenter/CoreMLSelfieSegmenter.hpp"
#endif

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::SelfieSegmenter;

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

namespace {

inline std::unique_ptr<ISelfieSegmenter> createSelfieSegmenter(const ILogger &logger, const PluginConfig &pluginConfig, int computeUnit, int numThreads)
{
	if (computeUnit == ComputeUnit::kDefault) {
		logger.info("Auto-detecting compute unit for selfie segmenter...");
#ifdef HAVE_COREML_SELFIE_SEGMENTER
		logger.info("Selecting CoreML on Apple platforms.");
		computeUnit = ComputeUnit::kCoreML;
#elif NCNN_VULKAN == 1
		if (ncnn::get_gpu_count() > 0) {
			logger.info("Vulkan-compatible GPU detected. Selecting NCNN Vulkan backend.");
			computeUnit = ComputeUnit::kNcnnVulkanGpu;
		} else {
			logger.info("No Vulkan-compatible GPU detected. Falling back to NCNN CPU.");
			computeUnit = ComputeUnit::kCPUOnly;
		}
#elif defined(__APPLE__)
		logger.info("Falling back to NCNN CPU backend due to lack of CoreML support.");
		computeUnit = ComputeUnit::kCPUOnly;
#else
		logger.info("Selecting CPU backend of NCNN.");
		computeUnit = ComputeUnit::kCPUOnly;
#endif
	}

	if (computeUnit & ComputeUnit::kCPUOnly != 0) {
		logger.info("Using NCNN CPU backend for selfie segmenter.");
		return std::make_unique<NcnnSelfieSegmenter>(pluginConfig.selfieSegmenterParamPath.c_str(),
						pluginConfig.selfieSegmenterBinPath.c_str(),
						numThreads);
	} else if (computeUnit & ComputeUnit::kNcnnVulkanGpu != 0) {
		int ncnnGpuIndex = computeUnit & 0xffff;
		logger.info("Using NCNN Vulkan GPU backend (GPU index: {}) for selfie segmenter.", ncnnGpuIndex);
		return std::make_unique<NcnnSelfieSegmenter>(pluginConfig.selfieSegmenterParamPath.c_str(),
						pluginConfig.selfieSegmenterBinPath.c_str(),
						numThreads, ncnnGpuIndex);
	} else if (computeUnit & ComputeUnit::kCoreML != 0) {
#ifdef HAVE_COREML_SELFIE_SEGMENTER
		logger.info("Using CoreML backend for selfie segmenter.");
		return std::make_unique<CoreMLSelfieSegmenter>();
#else
		logger.error("CoreML backend selected for selfie segmenter, but CoreML support is not compiled in.");
		return std::make_unique<NullSelfieSegmenter>();
#endif
	} else {
		logger.error("Unknown compute unit selected for selfie segmenter: {}. Using null segmenter.", computeUnit);
		return std::make_unique<NullSelfieSegmenter>();
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

RenderingContext::RenderingContext(
	obs_source_t *const _source,
	const BridgeUtils::ILogger &_logger,
	const MainEffect &_mainEffect,
	const BridgeUtils::ThrottledTaskQueue &_selfieSegmenterTaskQueue,
	const PluginConfig _pluginConfig,
	const std::uint32_t _subsamplingRate,
	const std::uint32_t width,
	const std::uint32_t height,
	const int computeUnit,
	const int ncnnNumThreads
)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
	  selfieSegmenterTaskQueue(_selfieSegmenterTaskQueue),
	  pluginConfig(_pluginConfig),
	  subsamplingRate(_subsamplingRate),
	  selfieSegmenter(std::make_unique<NcnnSelfieSegmenter>(pluginConfig.selfieSegmenterParamPath.c_str(),
								pluginConfig.selfieSegmenterBinPath.c_str(),
								ncnnGpuIndex, ncnnNumThreads)),
	  region{0, 0, width, height},
	  subRegion{0, 0, (region.width / subsamplingRate) & ~1u, (region.height / subsamplingRate) & ~1u},
	  maskRoi(getMaskRoiPosition(region.width, region.height, selfieSegmenter)),
	  bgrxOriginalImage(make_unique_gs_texture(region.width, region.height, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
	  r32fOriginalGrayscale(
		  make_unique_gs_texture(region.width, region.height, GS_R32F, 1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInput(make_unique_gs_texture(static_cast<std::uint32_t>(selfieSegmenter->getWidth()),
						    static_cast<std::uint32_t>(selfieSegmenter->getHeight()), GS_BGRX,
						    1, NULL, GS_RENDER_TARGET)),
	  bgrxSegmenterInputReader(static_cast<std::uint32_t>(selfieSegmenter->getWidth()),
				   static_cast<std::uint32_t>(selfieSegmenter->getHeight()), GS_BGRX),
	  segmenterInputBuffer(selfieSegmenter->getPixelCount() * 4),
	  r8SegmentationMask(make_unique_gs_texture(maskRoi.width, maskRoi.height, GS_R8, 1, NULL, GS_DYNAMIC)),
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
	  r8GFResult(make_unique_gs_texture(region.width, region.height, GS_R8, 1, NULL, GS_RENDER_TARGET)),
	  r8TimeAveragedMasks{make_unique_gs_texture(region.width, region.height, GS_R8, 1, NULL, GS_RENDER_TARGET),
			      make_unique_gs_texture(region.width, region.height, GS_R8, 1, NULL, GS_RENDER_TARGET)},
	  r32fGFTemporary1Sub(
		  make_unique_gs_texture(subRegion.width, subRegion.height, GS_R32F, 1, NULL, GS_RENDER_TARGET))
{
	selfieSegmenterNet.opt.use_vulkan_compute = ncnnGpuIndex >= 0;
	selfieSegmenterNet.opt.num_threads = ncnnNumThreads;
	selfieSegmenterNet.opt.use_local_pool_allocator = true;
	selfieSegmenterNet.opt.openmp_blocktime = 1;

	if (int ret = selfieSegmenterNet.load_param(pluginConfig.selfieSegmenterParamPath.c_str())) {
		throw std::runtime_error("Failed to load selfie segmenter param: " + std::to_string(ret));
	}

	if (int ret = selfieSegmenterNet.load_model(pluginConfig.selfieSegmenterBinPath.c_str())) {
		throw std::runtime_error("Failed to load selfie segmenter bin: " + std::to_string(ret));
	}
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

	gs_set_viewport(0, 0, region.width, region.height);
	gs_ortho(0.0f, (float)region.width, 0.0f, (float)region.height, -100.0f, 100.0f);
	gs_matrix_identity();

	obs_source_process_filter_end(source, mainEffect.effect.get(), region.width, region.height);
}

void RenderingContext::renderOriginalGrayscale(gs_texture_t *bgrxOriginalImage)
{
	mainEffect.convertToGrayscale(region.width, region.height, r32fOriginalGrayscale.get(), bgrxOriginalImage);
}

void RenderingContext::renderSegmenterInput(gs_texture_t *bgrxOriginalImage)
{
	RenderTargetGuard renderTargetGuard;
	TransformStateGuard transformStateGuard;

	gs_set_render_target_with_color_space(bgrxSegmenterInput.get(), nullptr, GS_CS_SRGB);

	gs_clear(GS_CLEAR_COLOR, &blackColor, 1.0f, 0);

	gs_set_viewport(maskRoi.x, maskRoi.y, maskRoi.width, maskRoi.height);
	gs_ortho(0.0f, static_cast<float>(region.width), 0.0f, static_cast<float>(region.height), -100.0f, 100.0f);
	gs_matrix_identity();

	mainEffect.draw(region.width, region.height, bgrxOriginalImage);
}

void RenderingContext::renderSegmentationMask()
{
	const std::uint8_t *segmentationMaskData =
		selfieSegmenter->getMask() + (maskRoi.y * selfieSegmenter->getWidth() + maskRoi.x);
	gs_texture_set_image(r8SegmentationMask.get(), segmentationMaskData,
			     static_cast<std::uint32_t>(selfieSegmenter->getWidth()), 0);
}

void RenderingContext::renderGuidedFilter(gs_texture_t *r16fOriginalGrayscale, gs_texture_t *r8SegmentationMask,
					  float eps)
{
	mainEffect.resampleByNearestR8(subRegion.width, subRegion.height, r8SubGFGuide.get(), r16fOriginalGrayscale);

	mainEffect.resampleByNearestR8(subRegion.width, subRegion.height, r8SubGFSource.get(), r8SegmentationMask);

	mainEffect.applyBoxFilterR8KS17(subRegion.width, subRegion.height, r32fSubGFMeanGuide.get(), r8SubGFGuide.get(),
					r32fGFTemporary1Sub.get());
	mainEffect.applyBoxFilterR8KS17(subRegion.width, subRegion.height, r32fSubGFMeanSource.get(),
					r8SubGFSource.get(), r32fGFTemporary1Sub.get());

	mainEffect.applyBoxFilterWithMulR8KS17(subRegion.width, subRegion.height, r32fSubGFMeanGuideSource.get(),
					       r8SubGFGuide.get(), r8SubGFSource.get(), r32fGFTemporary1Sub.get());
	mainEffect.applyBoxFilterWithSqR8KS17(subRegion.width, subRegion.height, r32fSubGFMeanGuideSq.get(),
					      r8SubGFGuide.get(), r32fGFTemporary1Sub.get());

	mainEffect.calculateGuidedFilterAAndB(subRegion.width, subRegion.height, r32fSubGFA.get(), r32fSubGFB.get(),
					      r32fSubGFMeanGuideSq.get(), r32fSubGFMeanGuide.get(),
					      r32fSubGFMeanGuideSource.get(), r32fSubGFMeanSource.get(), eps);

	mainEffect.finalizeGuidedFilter(region.width, region.height, r8GFResult.get(), r16fOriginalGrayscale,
					r32fSubGFA.get(), r32fSubGFB.get());
}

void RenderingContext::renderTimeAveragedMask(const unique_gs_texture_t &targetTexture,
					      const unique_gs_texture_t &previousMaskTexture,
					      const unique_gs_texture_t &sourceTexture, float alpha)
{
	mainEffect.timeAveragedFiltering(targetTexture, previousMaskTexture, sourceTexture, alpha);
}

void RenderingContext::kickSegmentationTask()
{
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

void RenderingContext::videoRender()
{
	FilterLevel _filterLevel = filterLevel;

	float _guidedFilterEps = guidedFilterEps;

	float _maskGamma = maskGamma;
	float _maskLowerBound = maskLowerBound;
	float _maskUpperBoundMargin = maskUpperBoundMargin;

	float _timeAveragedFilteringAlpha = timeAveragedFilteringAlpha;

	const bool needNewFrame = doesNextVideoRenderReceiveNewFrame;
	if (needNewFrame) {
		doesNextVideoRenderReceiveNewFrame = false;

		if (_filterLevel >= FilterLevel::Segmentation) {
			try {
				bgrxSegmenterInputReader.sync();
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
			renderGuidedFilter(r32fOriginalGrayscale.get(), r8SegmentationMask.get(), _guidedFilterEps);
		}

		if (_filterLevel >= FilterLevel::TimeAveragedFilter) {
			std::size_t nextIndex = 1 - currentTimeAveragedMaskIndex;
			renderTimeAveragedMask(r8TimeAveragedMasks[nextIndex],
					       r8TimeAveragedMasks[currentTimeAveragedMaskIndex], r8GFResult,
					       _timeAveragedFilteringAlpha);
			currentTimeAveragedMaskIndex = nextIndex;
		}
	}

	if (_filterLevel == FilterLevel::Passthrough) {
		mainEffect.draw(region.width, region.height, bgrxOriginalImage.get());
	} else if (_filterLevel == FilterLevel::Segmentation) {
		mainEffect.drawWithMask(region.width, region.height, bgrxOriginalImage.get(), r8SegmentationMask.get());
	} else if (_filterLevel == FilterLevel::GuidedFilter) {
		mainEffect.drawWithRefinedMask(region.width, region.height, bgrxOriginalImage.get(), r8GFResult.get(),
					       _maskGamma, _maskLowerBound, _maskUpperBoundMargin);
	} else if (_filterLevel == FilterLevel::TimeAveragedFilter) {
		mainEffect.drawWithRefinedMask(region.width, region.height, bgrxOriginalImage.get(),
					       r8TimeAveragedMasks[currentTimeAveragedMaskIndex].get(), _maskGamma,
					       _maskLowerBound, _maskUpperBoundMargin);
	} else {
		// Draw nothing to prevent unexpected background disclosure
	}

	if (needNewFrame && _filterLevel >= FilterLevel::Segmentation) {
		bgrxSegmenterInputReader.stage(bgrxSegmenterInput);
	}
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	return frame;
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
