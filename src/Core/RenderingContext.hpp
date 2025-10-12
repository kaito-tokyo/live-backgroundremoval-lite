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
#include <cstdint>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/net.h>
#else
#include <net.h>
#endif

#include "../BridgeUtils/AsyncTextureReader.hpp"
#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ILogger.hpp"
#include "../BridgeUtils/ThrottledTaskQueue.hpp"

#include "../SelfieSegmenter/SelfieSegmenter.hpp"

#include "MainEffect.hpp"
#include "PluginConfig.hpp"
#include "PluginProperty.hpp"

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

class RenderingContext : public std::enable_shared_from_this<RenderingContext> {
private:
	obs_source_t *const source;
	const BridgeUtils::ILogger &logger;
	const MainEffect &mainEffect;

	const PluginConfig pluginConfig;

	BridgeUtils::ThrottledTaskQueue &selfieSegmenterTaskQueue;
	ncnn::Net selfieSegmenterNet;
	SelfieSegmenter selfieSegmenter;
	BridgeUtils::AsyncTextureReader readerSegmenterInput;

public:
	const std::uint32_t subsamplingRate;
	const std::uint32_t width;
	const std::uint32_t height;
	const std::uint32_t widthSub;
	const std::uint32_t heightSub;

public:
	const BridgeUtils::unique_gs_texture_t bgrxOriginalImage;
	const BridgeUtils::unique_gs_texture_t r32fOriginalGrayscale;

	const BridgeUtils::unique_gs_texture_t bgrxSegmenterInput;

private:
	std::vector<std::uint8_t> segmenterInputBuffer;

public:
	const std::uint32_t maskRoiOffsetX;
	const std::uint32_t maskRoiOffsetY;
	const std::uint32_t maskRoiWidth;
	const std::uint32_t maskRoiHeight;

	const BridgeUtils::unique_gs_texture_t r8SegmentationMask;

public:
	const BridgeUtils::unique_gs_texture_t r8SubGFGuide;
	const BridgeUtils::unique_gs_texture_t r8SubGFSource;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuide;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanSource;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSource;
	const BridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSq;
	const BridgeUtils::unique_gs_texture_t r32fSubGFA;
	const BridgeUtils::unique_gs_texture_t r32fSubGFB;
	const BridgeUtils::unique_gs_texture_t r8GFResult;

	const std::array<BridgeUtils::unique_gs_texture_t, 2> r8TimeAveragedMasks;
	std::size_t currentTimeAveragedMaskIndex = 0;

private:
	const BridgeUtils::unique_gs_texture_t r32fGFTemporary1Sub;

public:
	std::atomic<FilterLevel> filterLevel;

	std::atomic<float> selfieSegmenterFps;

	std::atomic<float> guidedFilterEps;

	std::atomic<float> maskGamma;
	std::atomic<float> maskLowerBound;
	std::atomic<float> maskUpperBoundMargin;

	std::atomic<float> timeAveragedFilteringAlpha;

private:
	float timeSinceLastSelfieSegmentation = 0.0f;
	std::uint64_t lastFrameTimestamp = 0;
	std::atomic<bool> doesNextVideoRenderReceiveNewFrame = false;

	vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

public:
	RenderingContext(obs_source_t *source, const BridgeUtils::ILogger &logger, const MainEffect &mainEffect,
			 PluginConfig pluginConfig, BridgeUtils::ThrottledTaskQueue &selfieSegmenterTaskQueue,
			 int ncnnNumThreads, int ncnnGpuIndex, std::uint32_t subsamplingRate, std::uint32_t width,
			 std::uint32_t height);
	~RenderingContext() noexcept;

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);

	void applyPluginProperty(const PluginProperty &pluginProperty)
	{
		if (pluginProperty.filterLevel == FilterLevel::Default) {
			filterLevel = FilterLevel::TimeAveragedFilter;
		} else {
			filterLevel = pluginProperty.filterLevel;
		}

		selfieSegmenterFps = static_cast<float>(pluginProperty.selfieSegmenterFps);

		guidedFilterEps = static_cast<float>(std::pow(10.0, pluginProperty.guidedFilterEpsPowDb / 10.0));

		maskGamma = static_cast<float>(pluginProperty.maskGamma);
		maskLowerBound = static_cast<float>(std::pow(10.0, pluginProperty.maskLowerBoundAmpDb / 20.0));
		maskUpperBoundMargin =
			static_cast<float>(std::pow(10.0, pluginProperty.maskUpperBoundMarginAmpDb / 20.0));

		timeAveragedFilteringAlpha = static_cast<float>(pluginProperty.timeAveragedFilteringAlpha);
	}

private:
	void renderOriginalImage();
	void renderOriginalGrayscale(gs_texture_t *bgrxOriginalImage);
	void renderSegmenterInput(gs_texture_t *bgrxOriginalImage);
	void renderSegmentationMask();
	void calculateDifferenceWithMask();
	void renderGuidedFilter(gs_texture_t *r16fOriginalGrayscale, gs_texture_t *r8SegmentationMask, float eps);
	void renderTimeAveragedMask(const BridgeUtils::unique_gs_texture_t &targetTexture,
				    const BridgeUtils::unique_gs_texture_t &previousMaskTexture,
				    const BridgeUtils::unique_gs_texture_t &sourceTexture, float alpha);
	void kickSegmentationTask();
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
