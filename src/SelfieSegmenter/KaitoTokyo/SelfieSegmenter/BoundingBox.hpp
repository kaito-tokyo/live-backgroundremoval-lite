// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace KaitoTokyo::SelfieSegmenter {

struct BoundingBox {
	std::uint32_t x;
	std::uint32_t y;
	std::uint32_t width;
	std::uint32_t height;

	bool calculateBoundingBoxFrom256x144(const std::uint8_t *data, std::uint8_t threshold);
};

} // namespace KaitoTokyo::SelfieSegmenter
