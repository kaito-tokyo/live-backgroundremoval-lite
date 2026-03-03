// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>

namespace KaitoTokyo::Memory {

/**
 * @brief STL-compatible allocator that performs aligned allocations.
 *
 * This allocator can be used with standard containers (for example
 * `std::vector<T, AlignedAllocator<T>>`) when a specific alignment is
 * required for the stored objects.
 *
 * The alignment value is provided at construction time and stored in the
 * allocator instance; all allocations performed by this allocator use the
 * same alignment.
 *
 * @tparam T The value type to allocate.
 *
 * @note The caller is responsible for choosing an alignment that is supported
 *       by the platform and compatible with `T`. In practice this usually
 *       means:
 *       - the alignment is a non-zero power of two, and
 *       - the alignment is greater than or equal to `alignof(T)`.
 *       Passing an unsupported alignment may result in `std::bad_alloc` or
 *       undefined behavior.
 */
template<typename T> class AlignedAllocator {
public:
	using value_type = T;

	template<class U> struct rebind {
		using other = AlignedAllocator<U>;
	};

	explicit AlignedAllocator(std::size_t alignment) : alignment_(alignment)
	{
		if ((alignment & (alignment - 1)) != 0) {
			throw std::invalid_argument(
				"Alignment must be a power of two and at least alignof(std::max_align_t)");
		}
	}

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

	template<typename U> bool operator==(const AlignedAllocator<U> &other) const noexcept
	{
		return alignment_ == other.alignment();
	}

	template<typename U> bool operator!=(const AlignedAllocator<U> &other) const noexcept
	{
		return !(*this == other);
	}

private:
	std::size_t alignment_;
};

} // namespace KaitoTokyo::Memory
