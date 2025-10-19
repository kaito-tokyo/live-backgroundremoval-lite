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
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

class SelfieSegmenterMaskBuffer {
private:
	std::array<std::vector<std::uint8_t>, 2> buffers;
	std::atomic<std::size_t> readableIndex;
	mutable std::mutex bufferMutex;

public:
	explicit SelfieSegmenterMaskBuffer(std::size_t size)
		: buffers{std::vector<std::uint8_t>(size), std::vector<std::uint8_t>(size)},
		  readableIndex(0)
	{
	}

	void write(std::function<void(std::vector<std::uint8_t> &)> writeFunc)
	{
		std::lock_guard<std::mutex> lock(bufferMutex);
		const int writeIndex = (readableIndex.load(std::memory_order_relaxed) + 1) % 2;
		auto &buffer = buffers[writeIndex];
		writeFunc(buffer);
		readableIndex.store(writeIndex, std::memory_order_release);
	}

	const std::vector<std::uint8_t> &read() const { return buffers[readableIndex.load(std::memory_order_acquire)]; }

	SelfieSegmenterMaskBuffer(const SelfieSegmenterMaskBuffer &) = delete;
	SelfieSegmenterMaskBuffer &operator=(const SelfieSegmenterMaskBuffer &) = delete;
	SelfieSegmenterMaskBuffer(SelfieSegmenterMaskBuffer &&) = delete;
	SelfieSegmenterMaskBuffer &operator=(SelfieSegmenterMaskBuffer &&) = delete;
};

class ISelfieSegmenter {
protected:
	ISelfieSegmenter() = default;

public:
	virtual ~ISelfieSegmenter() = default;

	virtual std::size_t getWidth() const noexcept = 0;
	virtual std::size_t getHeight() const noexcept = 0;
	virtual std::size_t getPixelCount() const noexcept = 0;

	virtual void process(const std::uint8_t *bgraData) = 0;

	virtual const std::vector<std::uint8_t> &getMask() const = 0;

	ISelfieSegmenter(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter &operator=(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter(ISelfieSegmenter &&) = delete;
	ISelfieSegmenter &operator=(ISelfieSegmenter &&) = delete;
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
