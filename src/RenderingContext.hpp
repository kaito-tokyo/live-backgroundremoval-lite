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

#include "MainEffect.hpp"
#include "Preset.hpp"
#include "SelfieSegmenter.hpp"
#include "SelfieSegmenter/FrameExtractors.hpp"
#include "ThrottledTaskQueue.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class RenderingContext : public std::enable_shared_from_this<RenderingContext> {
private:
	obs_source_t *const source;
	const kaito_tokyo::obs_bridge_utils::ILogger &logger;
	const MainEffect &mainEffect;

	SelfieSegmenter selfieSegmenter;
	ThrottledTaskQueue &selfieSegmenterTaskQueue;

public:
	const std::uint32_t width;
	const std::uint32_t height;

public:
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxOriginalImage;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r16fOriginalGrayscale;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxSegmenterInput;

private:
	std::vector<std::uint8_t> segmenterInputBuffer;

public:
	const std::uint32_t maskRoiOffsetX;
	const std::uint32_t maskRoiOffsetY;
	const std::uint32_t maskRoiWidth;
	const std::uint32_t maskRoiHeight;

	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8SegmentationMask;

public:
	const std::uint32_t gfWidthSub;
	const std::uint32_t gfHeightSub;

	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8GFGuideSub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8GFSourceSub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFMeanGuideSub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFMeanSourceSub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFMeanGuideSourceSub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFMeanGuideSqSub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFASub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFBSub;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8GFResult;

private:
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r32fGFTemporary1Sub;

	const FilterLevel &filterLevel;
	const double &gfEps;
	const double &maskGamma;
	const double &maskLowerBound;
	const double &maskUpperBound;

	vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

	static constexpr int SUBSAMPLING_RATE = 4;

private:
	selfie_segmenter::NullFrameExtractor nullFrameExtractor;
	selfie_segmenter::UyvyLimitedRec709FrameExtractor uyvyLimitedRec709FrameExtractor;
	selfie_segmenter::UyvyFullRec709FrameExtractor uyvyFullRec709FrameExtractor;

public:
	RenderingContext(obs_source_t *source, const kaito_tokyo::obs_bridge_utils::ILogger &logger,
			 const MainEffect &mainEffect, const ncnn::Net &selfieSegmenterNet,
			 ThrottledTaskQueue &selfieSegmenterTaskQueue, std::uint32_t width, std::uint32_t height,
			 const FilterLevel &filterLevel, const double &gfEps, const double &maskGamma,
			 const double &maskLowerBound, const double &maskUpperBound);
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
