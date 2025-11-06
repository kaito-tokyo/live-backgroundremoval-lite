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
		if (pluginProperty.filterLevel == FilterLevel::Default) {
			if (pluginProperty.isStrictlySyncing) {
				filterLevel_ = FilterLevel::GuidedFilter;
			} else {
				filterLevel_ = FilterLevel::TimeAveragedFilter;
			}
			logger_.info("Default filter level is parsed to be {}", static_cast<int>(filterLevel_.load()));
		} else {
			filterLevel_ = pluginProperty.filterLevel;
			logger_.info("Filter level set to {}", static_cast<int>(filterLevel_.load()));
		}

		if (pluginProperty.selfieSegmenterFps == 0) {
			if (pluginProperty.isStrictlySyncing) {
				selfieSegmenterInterval_ = 1.0f / 15.0f;
			} else {
				selfieSegmenterInterval_ = 1.0f / 60.0f;
			}
			logger_.info("Default selfie segmenter interval is parsed to be {}",
				     selfieSegmenterInterval_.load());
		} else {
			selfieSegmenterInterval_ = 1.0f / static_cast<float>(pluginProperty.selfieSegmenterFps);
		}
		logger_.info("Selfie segmenter interval set to {}", selfieSegmenterInterval_.load());

		guidedFilterEps_ = static_cast<float>(std::pow(10.0, pluginProperty.guidedFilterEpsPowDb / 10.0));
		logger_.info("Guided filter epsilon set to {}", guidedFilterEps_.load());

		timeAveragedFilteringAlpha_ = static_cast<float>(pluginProperty.timeAveragedFilteringAlpha);
		logger_.info("Time-averaged filtering alpha set to {}", timeAveragedFilteringAlpha_.load());

		maskGamma_ = static_cast<float>(pluginProperty.maskGamma);
		logger_.info("Mask gamma set to {}", maskGamma_.load());

		maskLowerBound_ = static_cast<float>(std::pow(10.0, pluginProperty.maskLowerBoundAmpDb / 20.0));
		logger_.info("Mask lower bound set to {}", maskLowerBound_.load());

		maskUpperBoundMargin_ =
			static_cast<float>(std::pow(10.0, pluginProperty.maskUpperBoundMarginAmpDb / 20.0));
		logger_.info("Mask upper bound margin set to {}", maskUpperBoundMargin_.load());
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

public:
	const RenderingContextRegion region_;
	const RenderingContextRegion subRegion_;
	const RenderingContextRegion maskRoi_;

	const BridgeUtils::unique_gs_texture_t bgrxSource_;
	const BridgeUtils::unique_gs_texture_t r32fGrayscale_;

	const BridgeUtils::unique_gs_texture_t bgrxSegmenterInput_;
	BridgeUtils::AsyncTextureReader bgrxSegmenterInputReader_;

private:
	std::vector<std::uint8_t> segmenterInputBuffer_;

public:
	const BridgeUtils::unique_gs_texture_t r8SegmentationMask_;

private:
	const BridgeUtils::unique_gs_texture_t r32fSubGFIntermediate_;

public:
	const BridgeUtils::unique_gs_texture_t r8SubGFGuide_;
	const BridgeUtils::unique_gs_texture_t r8SubGFSource_;
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

	std::atomic<float> selfieSegmenterInterval_;

	std::atomic<float> guidedFilterEps_;

	std::atomic<float> maskGamma_;
	std::atomic<float> maskLowerBound_;
	std::atomic<float> maskUpperBoundMargin_;

	std::atomic<float> timeAveragedFilteringAlpha_;

private:
	float timeSinceLastSelfieSegmentation_ = 0.0f;
	std::atomic<bool> doesNextVideoRenderKickSelfieSegmentation_ = false;

	std::uint64_t lastFrameTimestamp_ = 0;
	std::atomic<bool> doesNextVideoRenderReceiveNewFrame_ = false;

	vec4 blackColor_ = {0.0f, 0.0f, 0.0f, 1.0f};
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
