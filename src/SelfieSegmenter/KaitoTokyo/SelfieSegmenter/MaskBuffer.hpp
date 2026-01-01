/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo SelfieSegmenter Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

#include <KaitoTokyo/Memory/AlignedAllocator.hpp>

namespace KaitoTokyo::SelfieSegmenter {

class MaskBuffer {
	using BlockType = std::vector<std::uint8_t, Memory::AlignedAllocator<std::uint8_t>>;

public:
	constexpr static std::size_t kAlignment = 32;

	explicit MaskBuffer(std::size_t size)
		: buffers_{BlockType(size, 0, Memory::AlignedAllocator<std::uint8_t>(kAlignment)),
			   BlockType(size, 0, Memory::AlignedAllocator<std::uint8_t>(kAlignment)),
			   BlockType(size, 0, Memory::AlignedAllocator<std::uint8_t>(kAlignment))}
	{
	}

	~MaskBuffer() noexcept = default;

	void write(std::function<void(std::uint8_t *)> writeFunc)
	{
		std::lock_guard<std::mutex> lock(bufferMutex_);
		auto &buffer = buffers_[writerIndex_];
		writeFunc(buffer.data());
		writerIndex_ = freshIndex_.exchange(writerIndex_);
		hasNewFrame_.store(true, std::memory_order_release);
	}

	const std::uint8_t *read() const
	{
		if (hasNewFrame_.exchange(false, std::memory_order_acq_rel)) {
			readerIndex_ = freshIndex_.exchange(readerIndex_, std::memory_order_acq_rel);
		}

		return buffers_[readerIndex_].data();
	}

	MaskBuffer(const MaskBuffer &) = delete;
	MaskBuffer &operator=(const MaskBuffer &) = delete;
	MaskBuffer(MaskBuffer &&) = delete;
	MaskBuffer &operator=(MaskBuffer &&) = delete;

private:
	std::array<BlockType, 3> buffers_;
	mutable std::size_t readerIndex_ = 0;
	std::size_t writerIndex_ = 1;
	mutable std::atomic<std::size_t> freshIndex_ = 2;
	mutable std::atomic<bool> hasNewFrame_ = false;
	mutable std::mutex bufferMutex_;
};

} // namespace KaitoTokyo::SelfieSegmenter
