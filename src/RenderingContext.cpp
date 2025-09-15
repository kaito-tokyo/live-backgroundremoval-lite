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

#include "RenderingContext.hpp"
#include "SelfieSegmenter.hpp"

using namespace kaito_tokyo::obs_bridge_utils;

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

RenderingContext::RenderingContext(std::uint32_t _width, std::uint32_t _height,
                   const ILogger &_logger) noexcept
    : width(_width), height(_height), logger(_logger),
      bgrxOriginalImage(make_unique_gs_texture(width, height, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
      bgrxSegmentorInput(make_unique_gs_texture(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT, GS_BGRX, 1, NULL, GS_RENDER_TARGET)),
      r8SegmentationMask(make_unique_gs_texture(width, height, GS_R8, 1, NULL, GS_DYNAMIC)),
      readerSegmenterInput(SelfieSegmenter::INPUT_WIDTH, SelfieSegmenter::INPUT_HEIGHT, GS_BGRX)
{
}

RenderingContext::~RenderingContext() noexcept
{
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
