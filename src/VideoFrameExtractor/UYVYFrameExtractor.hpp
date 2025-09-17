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
 * @brief Defines a set of header-only classes for extracting RGB data from UYVY video frames.
 * @details This file contains a policy-based base class and concrete implementations
 * for Rec. 709/Rec. 601 standards and Limited/Full color ranges.
 */

#pragma once

#include "IFrameExtractor.hpp"
#include <cstdint>
#include <algorithm> // For std::clamp

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

//==============================================================================
// 1. Policy Definitions
//==============================================================================

// --- 1a. Color Space Coefficients Policies ---
namespace coefficients {
    struct Rec709 {
        static constexpr float CR_TO_R = 1.5748f;
        static constexpr float CB_TO_G = -0.1873f;
        static constexpr float CR_TO_G = -0.4681f;
        static constexpr float CB_TO_B = 1.8556f;
    };
    struct Rec601 {
        static constexpr float CR_TO_R = 1.402f;
        static constexpr float CB_TO_G = -0.34414f;
        static constexpr float CR_TO_G = -0.71414f;
        static constexpr float CB_TO_B = 1.772f;
    };
} // namespace coefficients

// --- 1b. Color Range Scaling Policies ---
namespace range {
    /**
     * @struct Limited
     * @brief Scales YCbCr values from limited range (Y:16-235, C:16-240) to full range float.
     */
    struct Limited {
        static inline float scale_y(uint8_t y_val) {
            constexpr float Y_OFFSET = 16.0f;
            constexpr float Y_SCALE = 255.0f / (235.0f - 16.0f);
            return (static_cast<float>(y_val) - Y_OFFSET) * Y_SCALE;
        }
        static inline float scale_c(uint8_t c_val) {
            constexpr float C_OFFSET = 16.0f;
            constexpr float C_SCALE = 255.0f / (240.0f - 16.0f);
            return (static_cast<float>(c_val) - C_OFFSET) * C_SCALE;
        }
    };

    /**
     * @struct Full
     * @brief Scales YCbCr values from full range (0-255) to full range float (identity operation).
     */
    struct Full {
        static inline float scale_y(uint8_t y_val) {
            return static_cast<float>(y_val);
        }
        static inline float scale_c(uint8_t c_val) {
            return static_cast<float>(c_val);
        }
    };
} // namespace range


//==============================================================================
// 2. Templated Base Class (now with two policies)
//==============================================================================

/**
 * @class BaseUyvyFrameExtractor
 * @brief A policy-based base class for converting UYVY to RGB.
 * @tparam TColorCoefficients A policy class for YCbCr-to-RGB matrix coefficients.
 * @tparam TRangePolicy A policy class for scaling Y/Cb/Cr values from their source range.
 */
template <typename TColorCoefficients, typename TRangePolicy>
class BaseUyvyFrameExtractor : public IVideoFrameExtractor {
private:
    static inline float clamp_and_normalize(float val) {
        return std::clamp(val, 0.0f, 255.0f) / 255.0f;
    }

public:
    BaseUyvyFrameExtractor() = default;
    ~BaseUyvyFrameExtractor() override = default;

    void operator()(ChannelType dstR, ChannelType dstG, ChannelType dstB, DataType srcdata,
                std::size_t width, std::size_t height, std::size_t linesize) override {
        const auto *src = static_cast<const uint8_t *>(srcdata[0]);

        for (std::size_t y = 0; y < height; ++y) {
            const uint8_t *row_ptr = src + y * linesize;
            
            for (std::size_t x = 0; x < width; x += 2) {
                const uint8_t u_val  = row_ptr[x * 2 + 0];
                const uint8_t y0_val = row_ptr[x * 2 + 1];
                const uint8_t v_val  = row_ptr[x * 2 + 2];
                const uint8_t y1_val = row_ptr[x * 2 + 3];

                // 色範囲ポリシーを使ってYとCをスケーリング
                const float y0_f = TRangePolicy::scale_y(y0_val);
                const float y1_f = TRangePolicy::scale_y(y1_val);
                const float cb_f = TRangePolicy::scale_c(u_val);
                const float cr_f = TRangePolicy::scale_c(v_val);

                const float cb_centered = cb_f - 128.0f;
                const float cr_centered = cr_f - 128.0f;

                // 色空間係数ポリシーから係数を取得
                const float r_component = TColorCoefficients::CR_TO_R * cr_centered;
                const float g_component = TColorCoefficients::CB_TO_G * cb_centered + TColorCoefficients::CR_TO_G * cr_centered;
                const float b_component = TColorCoefficients::CB_TO_B * cb_centered;

                const float r0 = y0_f + r_component;
                const float g0 = y0_f + g_component;
                const float b0 = y0_f + b_component;
                
                const std::size_t idx0 = y * width + x;
                dstR[idx0] = clamp_and_normalize(r0);
                dstG[idx0] = clamp_and_normalize(g0);
                dstB[idx0] = clamp_and_normalize(b0);

                if (x + 1 < width) {
                    const float r1 = y1_f + r_component;
                    const float g1 = y1_f + g_component;
                    const float b1 = y1_f + b_component;
                    
                    const std::size_t idx1 = y * width + x + 1;
                    dstR[idx1] = clamp_and_normalize(r1);
                    dstG[idx1] = clamp_and_normalize(g1);
                    dstB[idx1] = clamp_and_normalize(b1);
                }
            }
        }
    }
};


//==============================================================================
// 3. Concrete Implementations (4 combinations)
//==============================================================================

// --- Rec. 709 based extractors ---
using UyvyLimitedRec709FrameExtractor = BaseUyvyFrameExtractor<coefficients::Rec709, range::Limited>;
using UyvyFullRec709FrameExtractor    = BaseUyvyFrameExtractor<coefficients::Rec709, range::Full>;

// --- Rec. 601 based extractors ---
using UyvyLimitedRec601FrameExtractor = BaseUyvyFrameExtractor<coefficients::Rec601, range::Limited>;
using UyvyFullRec601FrameExtractor    = BaseUyvyFrameExtractor<coefficients::Rec601, range::Full>;


} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
