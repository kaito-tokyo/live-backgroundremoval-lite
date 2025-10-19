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

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/net.h>
#else
#include <net.h>
#endif

#include "../BridgeUtils/AsyncTextureReader.hpp"
#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ILogger.hpp"
#include "../BridgeUtils/ThrottledTaskQueue.hpp"

#include "../SelfieSegmenter/ISelfieSegmenter.hpp"

#include "MainEffect.hpp"
#include "PluginConfig.hpp"
#include "PluginProperty.hpp"

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

struct RenderingContextRegion {
	std::uint32_t x;
	std::uint32_t y;
	std::uint32_t width;
	std::uint32_t height;
};

class RenderingContext : public std::enable_shared_from_this<RenderingContext> {
private:
	obs_source_t *const source;
	const BridgeUtils::ILogger &logger;
	const MainEffect &mainEffect;
	BridgeUtils::ThrottledTaskQueue &selfieSegmenterTaskQueue;
	const PluginConfig pluginConfig;

public:
	const std::uint32_t subsamplingRate;
	const int computeUnit;
	const int numThreads;

	std::unique_ptr<SelfieSegmenter::ISelfieSegmenter> selfieSegmenter;

public:
	const RenderingContextRegion region;
	const RenderingContextRegion subRegion;
	const RenderingContextRegion maskRoi;

	const BridgeUtils::unique_gs_texture_t bgrxSource;
	const BridgeUtils::unique_gs_texture_t r32fGrayscale;

	const BridgeUtils::unique_gs_texture_t bgrxSegmenterInput;
	BridgeUtils::AsyncTextureReader bgrxSegmenterInputReader;

private:
	std::vector<std::uint8_t> segmenterInputBuffer;

public:
	const BridgeUtils::unique_gs_texture_t r8SegmentationMask;

private:
	const BridgeUtils::unique_gs_texture_t r32fSubGFIntermediate;

public:
	const BridgeUtils::unique_gs_texture_t r8SubGFGuide;
	const BridgeUtils::unique_gs_texture_t r8SubGFSource;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuide;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanSource;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSource;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSq;
	const BridgeUtils::unique_gs_texture_t r32fSubGFA;
	const BridgeUtils::unique_gs_texture_t r32fSubGFB;
	const BridgeUtils::unique_gs_texture_t r8GuidedFilterResult;

	const std::array<BridgeUtils::unique_gs_texture_t, 2> r8TimeAveragedMasks;
	std::size_t currentTimeAveragedMaskIndex = 0;

public:
	std::atomic<FilterLevel> filterLevel;

	std::atomic<bool> isStrictlySyncing;

	std::atomic<float> selfieSegmenterInterval;

	std::atomic<float> guidedFilterEps;

	std::atomic<float> maskGamma;
	std::atomic<float> maskLowerBound;
	std::atomic<float> maskUpperBoundMargin;

	std::atomic<float> timeAveragedFilteringAlpha;

private:
	float timeSinceLastSelfieSegmentation = 0.0f;
	std::atomic<bool> doesNextVideoRenderKickSelfieSegmentation = false;

	std::uint64_t lastFrameTimestamp = 0;
	std::atomic<bool> doesNextVideoRenderReceiveNewFrame = false;

	vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

public:
	static int getActualComputeUnit(const BridgeUtils::ILogger &logger, int computeUnit)
	{

		if (computeUnit == ComputeUnit::kAuto) {
			logger.info("Automatically selecting compute unit...");
			logger.info("Selecting ncnn CPU backend because it is the most efficient.");
			return ComputeUnit::kCpuOnly;
		} else if ((computeUnit & ComputeUnit::kNcnnVulkanGpu) != 0) {
#ifdef NCNN_VULKAN
#if NCNN_VULKAN == 1
			int ncnnGpuCount = ncnn::get_gpu_count();
			if (ncnnGpuCount == 0) {
				logger.error(
					"ncnn Vulkan GPU backend selected, but no Vulkan-compatible GPU detected. Using null.");
				return ComputeUnit::kNull;
			}

			int ncnnGpuIndex = computeUnit & ComputeUnit::kNcnnVulkanGpuIndexMask;
			if (ncnnGpuIndex >= ncnnGpuCount) {
				logger.error(
					"ncnn Vulkan GPU backend selected with invalid GPU index {} (available GPUs: {}). Using null.",
					ncnnGpuIndex, ncnnGpuCount);
				return ComputeUnit::kNull;
			}

			return computeUnit;
#else
			logger.error(
				"ncnn Vulkan GPU backend selected, but ncnn is built without Vulkan support. Using null segmenter.");
			return ComputeUnit::kNull;
#endif
#else
			logger.error(
				"ncnn Vulkan GPU backend selected, but ncnn is built without Vulkan support. Using null.");
			return ComputeUnit::kNull;
#endif
		} else {
			return computeUnit;
		}
	}

	RenderingContext(obs_source_t *const source, const BridgeUtils::ILogger &logger, const MainEffect &mainEffect,
			 BridgeUtils::ThrottledTaskQueue &selfieSegmenterTaskQueue, const PluginConfig &pluginConfig,
			 const std::uint32_t subsamplingRate, const std::uint32_t width, const std::uint32_t height,
			 const int computeUnit, const int numThreads);
	~RenderingContext() noexcept;

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);

	void applyPluginProperty(const PluginProperty &pluginProperty)
	{
		isStrictlySyncing = pluginProperty.isStrictlySyncing;
		logger.info("Strict syncing is {}", isStrictlySyncing.load() ? "enabled" : "disabled");

		if (pluginProperty.filterLevel == FilterLevel::Default) {
			if (pluginProperty.isStrictlySyncing) {
				filterLevel = FilterLevel::GuidedFilter;
			} else {
				filterLevel = FilterLevel::TimeAveragedFilter;
			}
			logger.info("Default filter level is parsed to be {}", static_cast<int>(filterLevel.load()));
		} else {
			filterLevel = pluginProperty.filterLevel;
			logger.info("Filter level set to {}", static_cast<int>(filterLevel.load()));
		}

		if (pluginProperty.selfieSegmenterFps == 0) {
			if (pluginProperty.isStrictlySyncing) {
				selfieSegmenterInterval = 1.0f / 15.0f;
			} else {
				selfieSegmenterInterval = 1.0f / 60.0f;
			}
			logger.info("Default selfie segmenter interval is parsed to be {}",
				    selfieSegmenterInterval.load());
		} else {
			selfieSegmenterInterval = 1.0f / static_cast<float>(pluginProperty.selfieSegmenterFps);
		}
		logger.info("Selfie segmenter interval set to {}", selfieSegmenterInterval.load());

		guidedFilterEps = static_cast<float>(std::pow(10.0, pluginProperty.guidedFilterEpsPowDb / 10.0));
		logger.info("Guided filter epsilon set to {}", guidedFilterEps.load());

		timeAveragedFilteringAlpha = static_cast<float>(pluginProperty.timeAveragedFilteringAlpha);
		logger.info("Time-averaged filtering alpha set to {}", timeAveragedFilteringAlpha.load());

		maskGamma = static_cast<float>(pluginProperty.maskGamma);
		logger.info("Mask gamma set to {}", maskGamma.load());

		maskLowerBound = static_cast<float>(std::pow(10.0, pluginProperty.maskLowerBoundAmpDb / 20.0));
		logger.info("Mask lower bound set to {}", maskLowerBound.load());

		maskUpperBoundMargin =
			static_cast<float>(std::pow(10.0, pluginProperty.maskUpperBoundMarginAmpDb / 20.0));
		logger.info("Mask upper bound margin set to {}", maskUpperBoundMargin.load());
	}
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
