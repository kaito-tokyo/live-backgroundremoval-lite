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

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <net.h>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class SelfieSegmenter {
public:
    // Expose fixed dimensions as public constants
    static constexpr int INPUT_WIDTH = 256;
    static constexpr int INPUT_HEIGHT = 256;
    static constexpr int PIXEL_COUNT = INPUT_WIDTH * INPUT_HEIGHT;

    /**
     * @brief Constructor
     * @param param_path Path to the ncnn model's .param file
     * @param bin_path Path to the ncnn model's .bin file
     */
    SelfieSegmenter(const kaito_tokyo::obs_bridge_utils::unique_bfree_t &param_path,
                    const kaito_tokyo::obs_bridge_utils::unique_bfree_t &bin_path)
        : buffers(2, std::vector<uint8_t>(PIXEL_COUNT)),
          currentIndex(0)
    {
        if (net.load_param(param_path.get()) != 0) {
            throw std::runtime_error(std::string("Failed to load ncnn param file: ") + param_path.get());
        }
        if (net.load_model(bin_path.get()) != 0) {
            throw std::runtime_error(std::string("Failed to load ncnn bin file: ") + bin_path.get());
        }
    }

    ~SelfieSegmenter() {}

    /**
     * @brief Processes a 256x256 BGRA image to generate a background mask (for the writer thread).
     * @param bgra_data Pointer to the 256x256 BGRA image data.
     */
    void process(const uint8_t* bgra_data)
    {
        if (!bgra_data) {
            return;
        }

        // 1. Pre-processing (no resizing)
        // The input format is now BGRA. ncnn will handle the conversion to the model's required input format.
        ncnn::Mat in = ncnn::Mat::from_pixels(bgra_data, ncnn::Mat::PIXEL_BGRA2RGB, INPUT_WIDTH, INPUT_HEIGHT);
        in.substract_mean_normalize(meanVals, normVals);

        // 2. Run inference. This is the most time-consuming part and is done outside the lock
        // to maximize concurrency.
        ncnn::Extractor ex = net.create_extractor();
        ex.input("in0", in);
        ncnn::Mat out;
        ex.extract("out0", out); // Assuming the output size is also 256x256

        // Lock is acquired only for the short duration of writing to the buffer and updating the index.
        // This prevents race conditions with the reader thread (applyMaskToFrame).
        std::lock_guard<std::mutex> lock(bufferMutex);

        // 3. Post-processing & writing to the inactive buffer
        int next_idx = (currentIndex.load(std::memory_order_relaxed) + 1) % 2;
        std::vector<uint8_t>& target_buffer = buffers[next_idx];
        
        const float* src_ptr = out.channel(0);
        for (int i = 0; i < PIXEL_COUNT; i++) {
            target_buffer[i] = static_cast<uint8_t>(std::max(0.f, std::min(255.f, src_ptr[i] * 255.f)));
        }

        // 4. Atomically update the readable index. This is done inside the lock
        // to ensure the reader thread gets a consistent state.
        currentIndex.store(next_idx, std::memory_order_release);
    }

    /**
     * @brief Applies the latest mask directly to a 256x256 RGBA frame (overwrites the alpha channel).
     * This operation is zero-copy and thread-safe.
     * @param rgba_data Pointer to the RGBA (32-bit) frame data.
     */
    void applyMaskToFrame(uint8_t* rgba_data)
    {
        if (!rgba_data) return;

        // Lock the buffer to ensure thread-safe access to the mask data.
        // This prevents the writer thread (process) from modifying the buffer while we are reading it.
        std::lock_guard<std::mutex> lock(bufferMutex);

        // Get the index of the latest completed mask.
        // memory_order_relaxed is sufficient as the lock provides the necessary memory barrier.
        int read_idx = currentIndex.load(std::memory_order_relaxed);
        
        // Get a const reference to the mask data. No copy is made.
        const std::vector<uint8_t>& mask_data = buffers[read_idx];

        for (int i = 0; i < PIXEL_COUNT; ++i) {
            // Update the alpha channel of the RGBA data (every 4th byte) with the mask value
            rgba_data[i * 4 + 0] = 255;
            rgba_data[i * 4 + 1] = 255;
            rgba_data[i * 4 + 2] = 255;
            rgba_data[i * 4 + 3] = mask_data[i];
        }
    }

private:
    static constexpr float meanVals[3] = {127.5f, 127.5f, 127.5f};
    static constexpr float normVals[3] = {1.0f / 127.5f, 1.0f / 127.5f, 1.0f / 127.5f};
    
    std::vector<std::vector<uint8_t>> buffers;
    std::atomic<int> currentIndex;

    ncnn::Net net;

    std::mutex bufferMutex;
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
