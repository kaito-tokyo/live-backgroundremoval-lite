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

/**
 * @file UyvyFrameExtractor.hpp
 * @brief Defines a set of header-only classes for downsampling and extracting RGB data from UYVY frames.
 * @details This file contains a policy-based base class that uses the Nearest Neighbor algorithm
 * for downsampling, centering the result in a 256x256 canvas with black borders (letterboxing).
 * It includes policies for Rec. 709/Rec. 601 standards and Limited/Full color ranges.
 */

#pragma once

#include "IFrameExtractor.hpp"
#include <cstdint>
#include <algorithm> // For std::clamp, std::min, std::fill
#include <cmath>     // For std::round
#include <utility>   // For std::pair

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {
namespace selfie_segmenter {

//==============================================================================
// 0. Utility Function
//==============================================================================

using DownsampledDimensions = std::pair<std::size_t, std::size_t>;

inline DownsampledDimensions calculate_downsampled_dims(std::size_t src_width, std::size_t src_height,
							std::size_t max_dim = 256)
{
	if (src_width == 0 || src_height == 0)
		return {0, 0};
	const double aspect_ratio = static_cast<double>(src_width) / static_cast<double>(src_height);
	if (src_width > src_height) {
		const auto dst_width = std::min(src_width, max_dim);
		const auto dst_height = static_cast<std::size_t>(std::round(dst_width / aspect_ratio));
		return {dst_width, dst_height > 0 ? dst_height : 1};
	} else {
		const auto dst_height = std::min(src_height, max_dim);
		const auto dst_width = static_cast<std::size_t>(std::round(dst_height * aspect_ratio));
		return {dst_width > 0 ? dst_width : 1, dst_height};
	}
}

//==============================================================================
// 1. Policy Definitions
//==============================================================================

// --- 1a. Color Space Coefficients Policies ---
namespace coefficients {
struct Rec709 { /* HD TV */
	static constexpr float CR_TO_R = 1.5748f;
	static constexpr float CB_TO_G = -0.1873f;
	static constexpr float CR_TO_G = -0.4681f;
	static constexpr float CB_TO_B = 1.8556f;
};
struct Rec601 { /* SD TV */
	static constexpr float CR_TO_R = 1.402f;
	static constexpr float CB_TO_G = -0.34414f;
	static constexpr float CR_TO_G = -0.71414f;
	static constexpr float CB_TO_B = 1.772f;
};
} // namespace coefficients

// --- 1b. Color Range Scaling Policies ---
namespace range {
struct Limited {
	static inline float scale_y(uint8_t y_val) { return (static_cast<float>(y_val) - 16.0f) * (255.0f / 219.0f); }
	static inline float scale_c(uint8_t c_val) { return (static_cast<float>(c_val) - 128.0f); }
};
struct Full {
	static inline float scale_y(uint8_t y_val) { return static_cast<float>(y_val); }
	static inline float scale_c(uint8_t c_val) { return (static_cast<float>(c_val) - 128.0f); }
};
} // namespace range

//==============================================================================
// 2. Templated Base Class (with Centering/Letterboxing logic)
//==============================================================================

/**
 * @class BaseUyvyFrameExtractor
 * @brief A policy-based base class for downsampling and converting UYVY to RGB.
 * @details This implementation downsamples the source image using the Nearest Neighbor
 * method, preserves the aspect ratio, and centers the result in a 256x256
 * canvas, filling the borders with black.
 * @tparam TColorCoefficients A policy class for YCbCr-to-RGB matrix coefficients.
 * @tparam TRangePolicy A policy class for scaling Y/Cb/Cr values from their source range.
 */
template<typename TColorCoefficients, typename TRangePolicy>
class BaseUyvyFrameExtractor : public IVideoFrameExtractor {
public:
	BaseUyvyFrameExtractor() = default;
	~BaseUyvyFrameExtractor() override = default;

	void operator()(ChannelType dstR, ChannelType dstG, ChannelType dstB, DataArrayType srcdata, std::size_t width,
			std::size_t height, std::size_t linesize) override
	{

		constexpr std::size_t canvas_dim = 256;

		// 1. Calculate the dimensions of the inner, resized image
		const auto [img_width, img_height] = calculate_downsampled_dims(width, height, canvas_dim);
		if (img_width == 0 || img_height == 0) {
			// If source is invalid, fill the whole canvas with black and return
			std::fill(dstR, dstR + canvas_dim * canvas_dim, 0.0f);
			std::fill(dstG, dstG + canvas_dim * canvas_dim, 0.0f);
			std::fill(dstB, dstB + canvas_dim * canvas_dim, 0.0f);
			return;
		}

		// 2. Calculate offsets to center the image on the canvas
		const std::size_t offset_x = (canvas_dim - img_width) / 2;
		const std::size_t offset_y = (canvas_dim - img_height) / 2;

		// 3. Fill the entire canvas with black initially
		std::fill(dstR, dstR + canvas_dim * canvas_dim, 0.0f);
		std::fill(dstG, dstG + canvas_dim * canvas_dim, 0.0f);
		std::fill(dstB, dstB + canvas_dim * canvas_dim, 0.0f);

		// 4. Calculate scaling factors from source image to inner image
		const double scale_x = static_cast<double>(width) / img_width;
		const double scale_y = static_cast<double>(height) / img_height;
		const auto *src = static_cast<const uint8_t *>(srcdata[0]);

		// 5. Loop over the inner image area on the canvas
		for (std::size_t dy_img = 0; dy_img < img_height; ++dy_img) {
			const auto sy = std::min(static_cast<std::size_t>((dy_img + 0.5) * scale_y), height - 1);
			const uint8_t *const row_ptr = src + sy * linesize;
			const std::size_t canvas_y = offset_y + dy_img;
			const std::size_t dst_row_start_idx = canvas_y * canvas_dim + offset_x;

			for (std::size_t dx_img = 0; dx_img < img_width; ++dx_img) {
				// Find the nearest source pixel
				const auto sx = std::min(static_cast<std::size_t>((dx_img + 0.5) * scale_x), width - 1);

				// Fetch YUV of the single source pixel
				const uint8_t *chunk = &row_ptr[(sx / 2) * 4];
				const uint8_t u_val = chunk[0];
				const uint8_t v_val = chunk[2];
				const uint8_t y_val = (sx % 2 == 0) ? chunk[1] : chunk[3];

				// Scale Y and C values using the selected Range Policy
				const float y_f = TRangePolicy::scale_y(y_val);
				const float cb_centered = TRangePolicy::scale_c(u_val);
				const float cr_centered = TRangePolicy::scale_c(v_val);

				// Convert to RGB using the selected Color Coefficients Policy
				const float r_comp = TColorCoefficients::CR_TO_R * cr_centered;
				const float g_comp = TColorCoefficients::CB_TO_G * cb_centered +
						     TColorCoefficients::CR_TO_G * cr_centered;
				const float b_comp = TColorCoefficients::CB_TO_B * cb_centered;

				// Store the final, normalized RGB value at the correct canvas position
				const std::size_t dst_idx = dst_row_start_idx + dx_img;

				dstR[dst_idx] = std::clamp(y_f + r_comp, 0.0f, 255.0f) / 255.0f;
				dstG[dst_idx] = std::clamp(y_f + g_comp, 0.0f, 255.0f) / 255.0f;
				dstB[dst_idx] = std::clamp(y_f + b_comp, 0.0f, 255.0f) / 255.0f;
			}
		}
	}
};

//==============================================================================
// 3. Concrete Implementations (4 combinations)
//==============================================================================

// --- Rec. 709 based extractors (downsampling, letterboxed to 256x256) ---
using UyvyLimitedRec709FrameExtractor = BaseUyvyFrameExtractor<coefficients::Rec709, range::Limited>;
using UyvyFullRec709FrameExtractor = BaseUyvyFrameExtractor<coefficients::Rec709, range::Full>;

// --- Rec. 601 based extractors (downsampling, letterboxed to 256x256) ---
using UyvyLimitedRec601FrameExtractor = BaseUyvyFrameExtractor<coefficients::Rec601, range::Limited>;
using UyvyFullRec601FrameExtractor = BaseUyvyFrameExtractor<coefficients::Rec601, range::Full>;

} // namespace selfie_segmenter
} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
