// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>

namespace KaitoTokyo::SelfieSegmenter {

void copy_r8_bgra_to_float_chw(float *rChannel, float *gChannel, float *bChannel, const std::uint8_t *bgra_data,
			       const std::size_t pixelCount);

void copy_float32_to_r8(std::uint8_t *dst, const float *src, std::size_t pixelCount);

} // namespace KaitoTokyo::SelfieSegmenter
