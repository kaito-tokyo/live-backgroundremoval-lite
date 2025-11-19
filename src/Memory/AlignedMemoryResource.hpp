/*
 * MIT License
 * 
 * Copyright (c) 2025 Kaito Udagawa
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
#include <limits>
#include <memory>

#include <memory_resource>

namespace KaitoTokyo {
namespace Memory {

class AlignedMemoryResource : public std::pmr::memory_resource {
public:
	explicit AlignedMemoryResource(std::size_t alignment, std::pmr::memory_resource *upstream = nullptr)
		: alignment_(alignment),
		  upstream_(upstream ? upstream : std::pmr::get_default_resource())
	{
		if (alignment & (alignment - 1)) {
			throw std::invalid_argument("alignment must be a power of two");
		} else if (alignment < alignof(std::max_align_t)) {
			throw std::invalid_argument("alignment must be at least " +
						    std::to_string(alignof(std::max_align_t)));
		}
	}

protected:
	void *do_allocate(std::size_t bytes, std::size_t alignment) override
	{
		return upstream_->allocate(bytes, std::max(alignment, alignment_));
	}

	void do_deallocate(void *p, std::size_t bytes, std::size_t alignment) override
	{
		upstream_->deallocate(p, bytes, std::max(alignment, alignment_));
	}

	bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
	{
		if (this == &other) {
			return true;
		}

		if (typeid(*this) != typeid(other)) {
			return false;
		}

		auto otherPtr = static_cast<const AlignedMemoryResource *>(&other);

		return (alignment_ == otherPtr->alignment_) && (upstream_->is_equal(*otherPtr->upstream_));
	}

private:
	std::size_t alignment_;
	std::pmr::memory_resource *upstream_;
};

} // namespace Memory
} // namespace KaitoTokyo
