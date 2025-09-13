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

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <string>
#include <algorithm>

#include <net.h>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

/**
 * @class NcnnInferenceEngine
 * @brief A class responsible for loading and running ncnn models.
 */
class NcnnInferenceEngine {
public:
	NcnnInferenceEngine(const kaito_tokyo::obs_bridge_utils::unique_bfree_t &param_path,
			    const kaito_tokyo::obs_bridge_utils::unique_bfree_t &bin_path)
	{
		if (net.load_param(param_path.get()) != 0) {
			throw std::runtime_error(std::string("Failed to load ncnn param file: ") + param_path.get());
		}
		if (net.load_model(bin_path.get()) != 0) {
			throw std::runtime_error(std::string("Failed to load ncnn bin file: ") + bin_path.get());
		}
	}

	void run(const ncnn::Mat &input, ncnn::Mat &output) const
	{
		ncnn::Extractor ex = net.create_extractor();
		ex.input("in0", input);
		ex.extract("out0", output);
	}

private:
	ncnn::Net net;
};

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
	std::vector<std::uint8_t>& beginWrite()
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

	SelfieSegmenter(const kaito_tokyo::obs_bridge_utils::unique_bfree_t &param_path,
			const kaito_tokyo::obs_bridge_utils::unique_bfree_t &bin_path)
		: inferenceEngine(param_path, bin_path), maskBuffer(PIXEL_COUNT)
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
		inferenceEngine.run(m_inputMat, m_outputMat);

		// 3. Get a reference to the buffer to write to
		std::vector<std::uint8_t>& maskToWrite = maskBuffer.beginWrite();
		
		// 4. Post-process the result directly into the back buffer
		postprocess(maskToWrite);
		
		// 5. Commit the write, making the buffer available for reading
		maskBuffer.commitWrite();
	}

	/**
     * @brief Retrieves the latest segmentation mask (executed on the rendering thread).
     * @return A const reference to the vector of mask data (grayscale values).
     */
	const std::vector<std::uint8_t> &getMask() const
	{
		return maskBuffer.read();
	}

private:
	void preprocess(const std::uint8_t *bgra_data)
	{
		// Wrap the external pixel data without copying it.
		const ncnn::Mat in_external = ncnn::Mat(INPUT_WIDTH, INPUT_HEIGHT, (void *)bgra_data, 4, sizeof(uint8_t));

		// Convert from the external BGRA data to the internal RGB float Mat.
		m_inputMat.from_pixels(in_external, ncnn::Mat::PIXEL_BGRA2RGB, INPUT_WIDTH, INPUT_HEIGHT);
		m_inputMat.substract_mean_normalize(MEAN_VALS, NORM_VALS);
	}

	void postprocess(std::vector<std::uint8_t>& mask) const
	{
		const float *src_ptr = m_outputMat.channel(0);
		for (int i = 0; i < PIXEL_COUNT; i++) {
			mask[i] = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, src_ptr[i] * 255.f)));
		}
	}

	NcnnInferenceEngine inferenceEngine;
	MaskBuffer maskBuffer;

	// Member variables to reuse memory for ncnn::Mat
	ncnn::Mat m_inputMat;
	ncnn::Mat m_outputMat;

	static constexpr float MEAN_VALS[3] = {127.5f, 127.5f, 127.5f};
	static constexpr float NORM_VALS[3] = {1.0f / 127.5f, 1.0f / 127.5f, 1.0f / 127.5f};
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
