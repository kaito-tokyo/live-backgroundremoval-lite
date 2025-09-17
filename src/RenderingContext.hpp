/*
OBS Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <cstdint>

#include <net.h>

#include <obs-bridge-utils/gs_unique.hpp>
#include <obs-bridge-utils/ILogger.hpp>

#include "AsyncTextureReader.hpp"
#include "MainEffect.hpp"
#include "Preset.hpp"
#include "SelfieSegmenter.hpp"
#include "ThrottledTaskQueue.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class RenderingContext : public std::enable_shared_from_this<RenderingContext> {
private:
	obs_source_t *const source;
	const kaito_tokyo::obs_bridge_utils::ILogger &logger;
	const MainEffect &mainEffect;

	AsyncTextureReader readerSegmenterInput;
	SelfieSegmenter selfieSegmenter;
	ThrottledTaskQueue &selfieSegmenterTaskQueue;

public:
	const std::uint32_t width;
	const std::uint32_t height;
	const std::uint32_t subsamplingRate = 4;
	const std::uint32_t widthSub;
	const std::uint32_t heightSub;

public:
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxOriginalImage;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fOriginalGrayscale;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubOriginalGrayscale;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubLastOriginalGrayscale;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxSegmenterInput;

private:
	std::vector<std::uint8_t> segmenterInputBuffer;

public:
	const std::uint32_t maskRoiOffsetX;
	const std::uint32_t maskRoiOffsetY;
	const std::uint32_t maskRoiWidth;
	const std::uint32_t maskRoiHeight;

	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8SegmentationMask;

	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8SubGFGuide;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8SubGFSource;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubGFMeanGuide;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubGFMeanSource;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubGFMeanGuideSource;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubGFMeanGuideSq;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubGFA;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fSubGFB;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8GFResult;

private:
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFTemporary1Sub;

	const FilterLevel &filterLevel;
	const int &selfieSegmenterFps;
	const double &gfEps;
	const double &maskGamma;
	const double &maskLowerBound;
	const double &maskUpperBound;

	float timeSinceLastSelfieSegmentation = 0.0f;

	vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

public:
	RenderingContext(obs_source_t *source, const kaito_tokyo::obs_bridge_utils::ILogger &logger,
			 const MainEffect &mainEffect, const ncnn::Net &selfieSegmenterNet,
			 ThrottledTaskQueue &selfieSegmenterTaskQueue, std::uint32_t width, std::uint32_t height,
			 const FilterLevel &filterLevel, const int &selfieSegmenterFps, const double &gfEps,
			 const double &maskGamma, const double &maskLowerBound, const double &maskUpperBound);
	~RenderingContext() noexcept;

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);

private:
	void renderOriginalImage();
	void renderOriginalGrayscale(gs_texture_t *bgrxOriginalImage);
	void renderSegmenterInput(gs_texture_t *bgrxOriginalImage);
	void renderSegmentationMask();
	void renderGuidedFilter(gs_texture_t *r16fOriginalGrayscale, gs_texture_t *r8SegmentationMask);
	void kickSegmentationTask();
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo