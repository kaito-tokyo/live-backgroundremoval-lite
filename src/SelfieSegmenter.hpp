/*
Background Removal Lite
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

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <string>
#include <algorithm>

#include <net.h>

#include "BridgeUtils/ObsUnique.hpp"


namespace KaitoTokyo {
namespace BackgroundRemovalLite {

/**
 * @class MaskBuffer
 * @brief A thread-safe double buffer for managing mask data.
 */
class MaskBuffer {
public:
	explicit MaskBuffer(std::size_t size) : buffers(2, std::vector<std::uint8_t>(size)), readableIndex(0) {}

	/**
	 * @brief Locks the buffer and returns a reference to the back buffer for writing.
	 * The caller must call commitWrite() to make the buffer readable.
	 */
	std::vector<std::uint8_t> &beginWrite()
	{
		bufferMutex.lock();
		const int writeIndex = (readableIndex.load(std::memory_order_relaxed) + 1) % 2;
		return buffers[writeIndex];
	}

	/**
	 * @brief Commits the write operation, making the back buffer readable, and unlocks.
	 */
	void commitWrite()
	{
		const int writeIndex = (readableIndex.load(std::memory_order_relaxed) + 1) % 2;
		readableIndex.store(writeIndex, std::memory_order_release);
		bufferMutex.unlock();
	}

	const std::vector<std::uint8_t> &read() const
	{
		// Lock-free read
		return buffers[readableIndex.load(std::memory_order_acquire)];
	}

private:
	std::vector<std::vector<std::uint8_t>> buffers;
	std::atomic<int> readableIndex;
	mutable std::mutex bufferMutex;
};

/**
 * @class SelfieSegmenter
 * @brief A class that orchestrates image preprocessing, inference, and postprocessing.
 */
class SelfieSegmenter {
public:
	static constexpr int INPUT_WIDTH = 256;
	static constexpr int INPUT_HEIGHT = 256;
	static constexpr int PIXEL_COUNT = INPUT_WIDTH * INPUT_HEIGHT;

	static constexpr float MEAN_VALS[3] = {0.0f, 0.0f, 0.0f};
	static constexpr float NORM_VALS[3] = {1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f};

	const ncnn::Net &selfieSegmenterNet;
	MaskBuffer maskBuffer;

	// Member variables to reuse memory for ncnn::Mat
	ncnn::Mat m_inputMat;
	ncnn::Mat m_outputMat;

	SelfieSegmenter(const ncnn::Net &_selfieSegmenterNet)
		: selfieSegmenterNet(_selfieSegmenterNet),
		  maskBuffer(PIXEL_COUNT)
	{
		// Pre-allocate memory for the input Mat.
		m_inputMat.create(INPUT_WIDTH, INPUT_HEIGHT, 3, sizeof(float));
	}

	/**
     * @brief Generates a segmentation mask from BGRA image data (executed on the inference thread).
     * @param bgra_data A pointer to the 256x256 BGRA image data.
     */
	void process(const std::uint8_t *bgra_data)
	{
		if (!bgra_data) {
			return;
		}

		// 1. Pre-process data into the pre-allocated member Mat
		preprocess(bgra_data);

		// 2. Run inference using member Mats
		ncnn::Extractor ex = selfieSegmenterNet.create_extractor();
		ex.input("in0", m_inputMat);
		ex.extract("out0", m_outputMat);

		// 3. Get a reference to the buffer to write to
		std::vector<std::uint8_t> &maskToWrite = maskBuffer.beginWrite();

		// 4. Post-process the result directly into the back buffer
		postprocess(maskToWrite);

		// 5. Commit the write, making the buffer available for reading
		maskBuffer.commitWrite();
	}

	/**
     * @brief Retrieves the latest segmentation mask (executed on the rendering thread).
     * @return A const reference to the vector of mask data (grayscale values).
     */
	const std::vector<std::uint8_t> &getMask() const { return maskBuffer.read(); }

private:
	void preprocess(const std::uint8_t *bgra_data)
	{
		// Manually convert BGRA (uint8_t) to planar RGB (float) and apply normalization in a single loop.
		float *r_channel = m_inputMat.channel(0);
		float *g_channel = m_inputMat.channel(1);
		float *b_channel = m_inputMat.channel(2);

		for (int i = 0; i < PIXEL_COUNT; i++) {
			// BGRA layout and normalization formula: (pixel - mean) * norm
			// Since mean is 127.5f and norm is 1.0/127.5f, this is equivalent to (pixel / 127.5f) - 1.0f
			// However, to be precise, we follow the (pixel - mean) * norm calculation.
			b_channel[i] = (static_cast<float>(bgra_data[i * 4 + 0]) - MEAN_VALS[0]) * NORM_VALS[0];
			g_channel[i] = (static_cast<float>(bgra_data[i * 4 + 1]) - MEAN_VALS[1]) * NORM_VALS[1];
			r_channel[i] = (static_cast<float>(bgra_data[i * 4 + 2]) - MEAN_VALS[2]) * NORM_VALS[2];
			// Alpha channel (i*4 + 3) is ignored
		}
	}

	void postprocess(std::vector<std::uint8_t> &mask) const
	{
		const float *src_ptr = m_outputMat.channel(0);
		for (int i = 0; i < PIXEL_COUNT; i++) {
			mask[i] = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, src_ptr[i] * 255.f)));
		}
	}
};

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
