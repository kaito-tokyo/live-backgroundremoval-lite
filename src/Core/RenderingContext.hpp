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

#include <ILogger.hpp>

#include <AsyncTextureReader.hpp>
#include <GsUnique.hpp>

#include <ThrottledTaskQueue.hpp>

#include <ISelfieSegmenter.hpp>

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
public:
	RenderingContext(obs_source_t *const source, const Logger::ILogger &logger, const MainEffect &mainEffect,
			 TaskQueue::ThrottledTaskQueue &selfieSegmenterTaskQueue, const PluginConfig &pluginConfig,
			 const std::uint32_t subsamplingRate, const std::uint32_t width, const std::uint32_t height,
			 const int numThreads);
	~RenderingContext() noexcept;

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);

	void applyPluginProperty(const PluginProperty &pluginProperty)
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

		float newMotionIntensityThreshold = static_cast<float>(std::pow(10.0, pluginProperty.motionIntensityThresholdPowDb / 10.0));
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

		float newMaskUpperBoundMargin = static_cast<float>(std::pow(10.0, pluginProperty.maskUpperBoundMarginAmpDb / 20.0));
		maskUpperBoundMargin_.store(newMaskUpperBoundMargin, std::memory_order_release);
		logger_.info("Mask upper bound margin set to {}", newMaskUpperBoundMargin);
	}

private:
	obs_source_t *const source_;
	const Logger::ILogger &logger_;
	const MainEffect &mainEffect_;
	TaskQueue::ThrottledTaskQueue &selfieSegmenterTaskQueue_;
	const PluginConfig pluginConfig_;

public:
	const std::uint32_t subsamplingRate_;
	const int numThreads_;

	std::unique_ptr<SelfieSegmenter::ISelfieSegmenter> selfieSegmenter_;
	bool hasNewSegmenterInput_ = false;
	std::atomic<bool> hasNewSegmentationMask_ = false;

public:
	const RenderingContextRegion region_;
	const RenderingContextRegion subRegion_;
	const RenderingContextRegion subPaddedRegion_;
	const RenderingContextRegion maskRoi_;

	const BridgeUtils::unique_gs_texture_t bgrxSource_;
	const BridgeUtils::unique_gs_texture_t r32fLuma_;

	const std::array<BridgeUtils::unique_gs_texture_t, 2> r32fSubLumas_;
	std::size_t currentSubLumaIndex_ = 0;

	const BridgeUtils::unique_gs_texture_t r32fSubPaddedSquaredMotion_;
	const std::vector<BridgeUtils::unique_gs_texture_t> r32fMeanSquaredMotionReductionPyramid_;
	BridgeUtils::AsyncTextureReader r32fReducedMeanSquaredMotionReader_;

	const BridgeUtils::unique_gs_texture_t bgrxSegmenterInput_;
	BridgeUtils::AsyncTextureReader bgrxSegmenterInputReader_;

private:
	std::vector<std::uint8_t> segmenterInputBuffer_;

public:
	const BridgeUtils::unique_gs_texture_t r8SegmentationMask_;

private:
	const BridgeUtils::unique_gs_texture_t r32fSubGFIntermediate_;

public:
	const BridgeUtils::unique_gs_texture_t r32fSubGFSource_;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuide_;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanSource_;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSource_;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSq_;
	const BridgeUtils::unique_gs_texture_t r32fSubGFA_;
	const BridgeUtils::unique_gs_texture_t r32fSubGFB_;
	const BridgeUtils::unique_gs_texture_t r8GuidedFilterResult_;

	const std::array<BridgeUtils::unique_gs_texture_t, 2> r8TimeAveragedMasks_;
	std::size_t currentTimeAveragedMaskIndex_ = 0;

public:
	std::atomic<FilterLevel> filterLevel_;

	std::atomic<float> motionIntensityThreshold_;

	std::atomic<float> guidedFilterEps_;

	std::atomic<float> maskGamma_;
	std::atomic<float> maskLowerBound_;
	std::atomic<float> maskUpperBoundMargin_;

	std::atomic<float> timeAveragedFilteringAlpha_;

private:
	std::uint64_t lastFrameTimestamp_ = 0;
	float timeSinceLastProcessFrame_ = 0.0f;
	constexpr static float kMaximumIntervalSecondsBetweenProcessFrames_ = 1.0f;
	std::atomic<bool> shouldNextVideoRenderProcessFrame_ = true;

	vec4 blackColor_ = {0.0f, 0.0f, 0.0f, 1.0f};
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
