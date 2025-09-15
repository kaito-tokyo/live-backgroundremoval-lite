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

#include <obs-bridge-utils/gs_unique.hpp>
#include <obs-bridge-utils/ILogger.hpp>

#include "AsyncTextureReader.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class RenderingContext {
public:
    const std::uint32_t width;
    const std::uint32_t height;
    const kaito_tokyo::obs_bridge_utils::ILogger &logger;

	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxOriginalImage;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxSegmentorInput;
	const kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8SegmentationMask;

    const AsyncTextureReader readerSegmenterInput;

    RenderingContext(std::uint32_t width, std::uint32_t height, const kaito_tokyo::obs_bridge_utils::ILogger &logger) noexcept;
    ~RenderingContext() noexcept;
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
