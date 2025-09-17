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
 * @file IFrameExtractor.hpp
 * @brief Defines an interface for extracting RGB data from a video frame.
 */

#pragma once

#include <cstddef>

#if defined(__GNUC__) || defined(__clang__)
/**
     * @def VIDEO_FRAME_EXTRACTOR_RESTRICT
     * @brief A macro that defines the __restrict__ keyword for GCC/Clang compilers.
     */
#define VIDEO_FRAME_EXTRACTOR_RESTRICT __restrict__
#elif defined(_MSC_VER)
/**
     * @def VIDEO_FRAME_EXTRACTOR_RESTRICT
     * @brief A macro that defines __declspec(restrict) for the MSVC compiler.
     */
#define VIDEO_FRAME_EXTRACTOR_RESTRICT __declspec(restrict)
#else
/**
     * @def VIDEO_FRAME_EXTRACTOR_RESTRICT
     * @brief A macro that expands to nothing for unsupported compilers.
     */
#define VIDEO_FRAME_EXTRACTOR_RESTRICT
#endif

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

/**
 * @class IVideoFrameExtractor
 * @brief An interface for extracting color information from a video frame and separating it into R, G, and B channels.
 *
 * This class is an abstract base class for handling pixel data from various
 * video formats (e.g., UYVY, NV12) in a unified way.
 * Derived classes implement the specific extraction logic for a particular
 * video format in operator().
 * @note For performance improvement, a macro is used for the __restrict__ keyword
 * (or a compatible feature) to inform the compiler that pointers do not alias.
 */
class IVideoFrameExtractor {
protected:
    /// @brief A type alias for a pointer to a destination channel buffer, qualified with restrict.
    using ChannelType = float *VIDEO_FRAME_EXTRACTOR_RESTRICT;
    /// @brief A type alias for the source video data planes, qualified with restrict.
    using DataType = const void *VIDEO_FRAME_EXTRACTOR_RESTRICT[8];

public:
    /**
     * @brief Virtual destructor.
     * @details Ensures that instances of derived classes are safely destroyed when
     * deleted via a base class pointer.
     */
    virtual ~IVideoFrameExtractor() = default;

    /**
     * @brief Processes video frame data and converts it to RGB channels.
     * @details This is a pure virtual function to be implemented by derived classes.
     * It reads the source data, converts it to RGB, and stores the result
     * in the respective channel buffers.
     *
     * @param[out] dstR     A pointer to the destination buffer for the Red channel. Values will be normalized to the range [0.0f, 1.0f].
     * @param[out] dstG     A pointer to the destination buffer for the Green channel. Values will be normalized to the range [0.0f, 1.0f].
     * @param[out] dstB     A pointer to the destination buffer for the Blue channel. Values will be normalized to the range [0.0f, 1.0f].
     * @param[in]  srcdata  An array of pointers to the source video frame data planes. Depending on the format, srcdata[0] could be the luma (Y) plane, srcdata[1] the chroma (UV) plane, etc.
     * @param[in]  width    The logical width of the frame in pixels.
     * @param[in]  height   The logical height of the frame in pixels.
     * @param[in]  linesize The number of bytes in one horizontal line of the source data (the stride). Due to memory alignment, this may differ from width * bytes_per_pixel.
     */
    virtual void operator()(ChannelType dstR, ChannelType dstG, ChannelType dstB, DataType srcdata,
                std::size_t width, std::size_t height, std::size_t linesize) = 0;
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
