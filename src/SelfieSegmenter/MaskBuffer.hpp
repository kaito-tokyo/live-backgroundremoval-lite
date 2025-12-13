/*
 * SelfieSegmenter
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

#include <AlignedMemoryResource.hpp>

namespace KaitoTokyo {
namespace SelfieSegmenter {

class MaskBuffer {
public:
	constexpr static std::size_t kAlignment = 32;

	explicit MaskBuffer(std::size_t size)
		: memoryResource_(kAlignment),
		  buffers_{std::pmr::vector<std::uint8_t>(size, 0, {&memoryResource_}),
			   std::pmr::vector<std::uint8_t>(size, 0, {&memoryResource_}),
			   std::pmr::vector<std::uint8_t>(size, 0, {&memoryResource_})}
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
	Memory::AlignedMemoryResource memoryResource_;

	std::array<std::pmr::vector<std::uint8_t>, 3> buffers_;
	mutable std::size_t readerIndex_ = 0;
	std::size_t writerIndex_ = 1;
	mutable std::atomic<std::size_t> freshIndex_ = 2;
	mutable std::atomic<bool> hasNewFrame_ = false;
	mutable std::mutex bufferMutex_;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
