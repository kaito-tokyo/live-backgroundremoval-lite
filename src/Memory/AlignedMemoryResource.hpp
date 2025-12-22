/*
 * KaitoTokyo Memory Library
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
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
