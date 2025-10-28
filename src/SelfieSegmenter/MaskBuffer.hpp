/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

#include "ForceAlignmentResource.hpp"

namespace KaitoTokyo {
namespace SelfieSegmenter {

class MaskBuffer {
public:
	constexpr static std::size_t kAlignment = 32;

	explicit MaskBuffer(std::size_t size)
		: defaultResource_(*std::pmr::new_delete_resource()),
		  alignedResource_(kAlignment, &defaultResource_),
		  buffers_{std::pmr::vector<std::uint8_t>(size, {&alignedResource_}),
			  std::pmr::vector<std::uint8_t>(size, {&alignedResource_})}
	{
	}

	void write(std::function<void(std::uint8_t *)> writeFunc)
	{
		std::lock_guard<std::mutex> lock(bufferMutex_);
		const int writeIndex = (readableIndex_.load(std::memory_order_relaxed) + 1) % 2;
		auto &buffer = buffers_[writeIndex];
		writeFunc(buffer.data());
		readableIndex_.store(writeIndex, std::memory_order_release);
	}

	const std::uint8_t *read() const { return buffers_[readableIndex_.load(std::memory_order_acquire)].data(); }

	MaskBuffer(const MaskBuffer &) = delete;
	MaskBuffer &operator=(const MaskBuffer &) = delete;
	MaskBuffer(MaskBuffer &&) = delete;
	MaskBuffer &operator=(MaskBuffer &&) = delete;

private:
	std::pmr::memory_resource &defaultResource_;
	ForceAlignmentResource alignedResource_;

	std::array<std::pmr::vector<std::uint8_t>, 2> buffers_;
	std::atomic<std::size_t> readableIndex_ = 0;
	mutable std::mutex bufferMutex_;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
