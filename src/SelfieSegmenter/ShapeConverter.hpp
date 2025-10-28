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

#include <cstddef>
#include <cstdint>

namespace KaitoTokyo {
namespace SelfieSegmenter {

void copy_r8_bgra_to_float_chw(float *rChannel, float *gChannel, float *bChannel, const std::uint8_t *bgra_data,
			       const std::size_t pixelCount);

void copy_float32_to_r8(std::uint8_t *dst, const float *src, std::size_t pixelCount);

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
