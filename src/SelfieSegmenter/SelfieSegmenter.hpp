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

namespace KaitoTokyo {
namespace BackgroundRemovalLite {
namespace SelfieSegmenterDetail {

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

} // namespace SelfieSegmenterDetail

/**
 * @class SelfieSegmenter
 * @brief A class that orchestrates image preprocessing, inference, and postprocessing.
 */
class SelfieSegmenter {
public:
	static constexpr int INPUT_WIDTH = 256;
	static constexpr int INPUT_HEIGHT = 256;
	static constexpr int PIXEL_COUNT = INPUT_WIDTH * INPUT_HEIGHT;

private:
	const ncnn::Net &selfieSegmenterNet;
	SelfieSegmenterDetail::MaskBuffer maskBuffer;
	ncnn::Mat inputMat;
	ncnn::Mat outputMat;
	bool isAVX2Available;

public:
	SelfieSegmenter(const ncnn::Net &_selfieSegmenterNet);
	void process(const std::uint8_t *bgra_data);
	const std::vector<std::uint8_t> &getMask() const;

private:
	void preprocess(const std::uint8_t *bgra_data);
	void postprocess(std::vector<std::uint8_t> &mask) const;
};

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
