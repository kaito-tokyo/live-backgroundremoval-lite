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

#include "Allocator.hpp"

namespace KaitoTokyo {
namespace SelfieSegmenter {

class MaskBuffer {
public:
	static constexpr std::size_t ALIGNMENT = 32;

private:
	std::pmr::memory_resource &defaultResource;
	ForceAlignmentResource alignedResource;
	;

	std::array<std::pmr::vector<std::uint8_t>, 2> buffers;
	std::atomic<std::size_t> readableIndex;
	mutable std::mutex bufferMutex;

public:
	explicit MaskBuffer(std::size_t size)
		: defaultResource(*std::pmr::new_delete_resource()),
		  alignedResource(ALIGNMENT, &defaultResource),
		  buffers{std::pmr::vector<std::uint8_t>(size, {&alignedResource}),
			  std::pmr::vector<std::uint8_t>(size, {&alignedResource})},
		  readableIndex(0)
	{
	}

	void write(std::function<void(std::uint8_t *)> writeFunc)
	{
		std::lock_guard<std::mutex> lock(bufferMutex);
		const int writeIndex = (readableIndex.load(std::memory_order_relaxed) + 1) % 2;
		auto &buffer = buffers[writeIndex];
		writeFunc(buffer.data());
		readableIndex.store(writeIndex, std::memory_order_release);
	}

	const std::uint8_t *read() const
	{
		return buffers[readableIndex.load(std::memory_order_acquire)].data();
		;
	}

	MaskBuffer(const MaskBuffer &) = delete;
	MaskBuffer &operator=(const MaskBuffer &) = delete;
	MaskBuffer(MaskBuffer &&) = delete;
	MaskBuffer &operator=(MaskBuffer &&) = delete;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
