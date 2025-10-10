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

#include <ncnn/net.h>

#include "../BridgeUtils/AsyncTextureReader.hpp"
#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ILogger.hpp"
#include "../BridgeUtils/ThrottledTaskQueue.hpp"

#include "../SelfieSegmenter/SelfieSegmenter.hpp"

#include "FilterLevel.hpp"
#include "MainEffect.hpp"
#include "PluginConfig.hpp"

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

class RenderingContext : public std::enable_shared_from_this<RenderingContext> {
private:
	obs_source_t *const source;
	const BridgeUtils::ILogger &logger;
	const MainEffect &mainEffect;

	BridgeUtils::AsyncTextureReader readerSegmenterInput;
	SelfieSegmenter selfieSegmenter;
	BridgeUtils::ThrottledTaskQueue &selfieSegmenterTaskQueue;

public:
	const PluginConfig pluginConfig;
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

private:
	const BridgeUtils::unique_gs_texture_t r32fGFTemporary1Sub;

public:
	std::atomic<FilterLevel> filterLevel;

	std::atomic<float> selfieSegmenterFps;

	std::atomic<float> gfEps;

	std::atomic<float> maskGamma;
	std::atomic<float> maskLowerBound;
	std::atomic<float> maskUpperBoundMargin;

private:
	float timeSinceLastSelfieSegmentation = 0.0f;
	std::uint64_t lastFrameTimestamp = 0;
	std::atomic<bool> doesNextVideoRenderReceiveNewFrame = false;

	vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

public:
	RenderingContext(obs_source_t *source, const BridgeUtils::ILogger &logger, const MainEffect &mainEffect,
			 const ncnn::Net &selfieSegmenterNet, BridgeUtils::ThrottledTaskQueue &selfieSegmenterTaskQueue,
			 PluginConfig pluginConfig, std::uint32_t subsamplingRate, std::uint32_t width,
			 std::uint32_t height);
	~RenderingContext() noexcept;

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);

private:
	void renderOriginalImage();
	void renderOriginalGrayscale(gs_texture_t *bgrxOriginalImage);
	void renderSegmenterInput(gs_texture_t *bgrxOriginalImage);
	void renderSegmentationMask();
	void calculateDifferenceWithMask();
	void renderGuidedFilter(gs_texture_t *r16fOriginalGrayscale, gs_texture_t *r8SegmentationMask, float gfEps);
	void kickSegmentationTask();
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
