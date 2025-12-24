/*
 * KaitoTokyo Async Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace KaitoTokyo::Memory {

template<typename T> class AlignedAllocator {
public:
	using value_type = T;

	explicit AlignedAllocator(std::size_t alignment) noexcept : alignment_(alignment) {}

	template<class U> AlignedAllocator(const AlignedAllocator<U> &other) noexcept : alignment_(other.alignment()) {}

	T *allocate(std::size_t n)
	{
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
			throw std::bad_alloc();
		}

		std::size_t bytes = n * sizeof(T);
		void *p = ::operator new(bytes, std::align_val_t(alignment_));
		return static_cast<T *>(p);
	}

	void deallocate(T *p, std::size_t) noexcept { ::operator delete(p, std::align_val_t(alignment_)); }

	std::size_t alignment() const noexcept { return alignment_; }

	bool operator==(const AlignedAllocator &other) const noexcept { return alignment_ == other.alignment_; }

	bool operator!=(const AlignedAllocator &other) const noexcept { return !(*this == other); }

private:
	std::size_t alignment_;
};

} // namespace KaitoTokyo::Memory
