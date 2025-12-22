/*
 * SelfieSegmenter
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
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
