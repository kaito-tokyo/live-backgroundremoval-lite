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
 * @file UYVYFrameExtractor.hpp
 * @brief Defines the implementation of the frame extractor for the UYVY video format.
 */

#pragma once

#include "IFrameExtractor.hpp"
#include <cstdint>

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

/**
 * @class UYVYFrameExtractor
 * @brief An extractor that converts frame data from the UYVY (YUV 4:2:2) video format to RGB.
 * @details This is a concrete implementation of the IVideoFrameExtractor interface.
 * This class reads pixel data in UYVY format (representing 2 pixels in 4 bytes)
 * and converts it into separate R, G, B channel data at high speed using
 * optimized integer arithmetic.
 */
class UYVYFrameExtractor : public IVideoFrameExtractor {
private:
    /**
     * @brief Fixed-point coefficients for YUV-to-RGB conversion (original coefficient * 256).
     * @details These are used to avoid floating-point operations and perform calculations
     * with high-speed integer arithmetic and bit shifts.
     */
    static constexpr int COEFF_R_V = 359;
    static constexpr int COEFF_G_U = -88;
    static constexpr int COEFF_G_V = -183;
    static constexpr int COEFF_B_U = 454;

    /// @brief A compile-time constant for normalizing 0-255 values to [0.0f, 1.0f] (1.0f / 255.0f).
    static constexpr float INV_255 = 1.0f / 255.0f;

    /**
     * @brief Clamps an integer value to the [0, 255] range.
     * @param[in] value The integer value to be clamped.
     * @return An unsigned 8-bit integer clamped to the range 0 to 255.
     */
    inline std::uint8_t clamp_u8(int value) {
        if (value < 0) return 0;
        if (value > 255) return 255;
        return static_cast<std::uint8_t>(value);
    }

public:
    /**
     * @brief Processes a UYVY formatted frame and converts it to RGB channels.
     * @details This is the implementation of IVideoFrameExtractor::operator().
     * Since UYVY data constitutes 2 pixels in 4 bytes, it reads 4 bytes at a time
     * within the loop, simultaneously calculates the RGB values for two pixels,
     * and writes them to the output buffers.
     * @copydoc IVideoFrameExtractor::operator()
     */
    void operator()(ChannelType rChannel, ChannelType gChannel, ChannelType bChannel, DataType data, std::size_t width, std::size_t height, std::size_t linesize) override {
        // Cast the generic 'const void*' from DataType to the specific 'const uint8_t*' needed for UYVY processing.
        const auto *uyvy_base_ptr = static_cast<const std::uint8_t *>(data[0]);

        for (std::size_t y = 0; y < height; ++y) {
            const std::uint8_t *uyvy_row = uyvy_base_ptr + y * linesize;
            
            const std::size_t row_offset = y * width;
            float *r_ptr = rChannel + row_offset;
            float *g_ptr = gChannel + row_offset;
            float *b_ptr = bChannel + row_offset;

            for (std::size_t x = 0; x < width / 2; ++x) {
                const int u  = static_cast<int>(uyvy_row[x * 4 + 0]);
                const int y0 = static_cast<int>(uyvy_row[x * 4 + 1]);
                const int v  = static_cast<int>(uyvy_row[x * 4 + 2]);
                const int y1 = static_cast<int>(uyvy_row[x * 4 + 3]);
                
                const int c_b = u - 128;
                const int c_r = v - 128;

                const int term_r = COEFF_R_V * c_r;
                const int term_g = COEFF_G_U * c_b + COEFF_G_V * c_r;
                const int term_b = COEFF_B_U * c_b;
                
                const int scaled_y0 = y0 << 8;
                const int scaled_y1 = y1 << 8;

                const int r0_i = (scaled_y0 + term_r) >> 8;
                const int g0_i = (scaled_y0 + term_g) >> 8;
                const int b0_i = (scaled_y0 + term_b) >> 8;

                const int r1_i = (scaled_y1 + term_r) >> 8;
                const int g1_i = (scaled_y1 + term_g) >> 8;
                const int b1_i = (scaled_y1 + term_b) >> 8;

                *r_ptr++ = clamp_u8(r0_i) * INV_255;
                *g_ptr++ = clamp_u8(g0_i) * INV_255;
                *b_ptr++ = clamp_u8(b0_i) * INV_255;

                *r_ptr++ = clamp_u8(r1_i) * INV_255;
                *g_ptr++ = clamp_u8(g1_i) * INV_255;
                *b_ptr++ = clamp_u8(b1_i) * INV_255;
            }
        }
    }
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
