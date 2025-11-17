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
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <vector>

#include <ILogger.hpp>

namespace KaitoTokyo {
namespace TaskQueue {

/**
 * @class MemoryBlockPool
 * @brief A thread-safe memory pool for fixed-size, aligned memory blocks.
 *
 * This pool manages a collection of memory blocks of a specific size and alignment.
 * It uses std::shared_ptr with a custom deleter to automatically return blocks
 * to the pool when they are no longer in use.
 *
 * The pool's lifecycle is managed by std::enable_shared_from_this, ensuring that
 * blocks can be safely returned to the pool (or freed if the pool is destroyed)
 * even if the pool instance itself is destroyed while blocks are still in use.
 */
class MemoryBlockPool : public std::enable_shared_from_this<MemoryBlockPool> {
public:
	/**
	 * @brief A shared pointer to a memory block acquired from the pool.
	 *
	 * When this pointer goes out of scope, the underlying memory block is
	 * automatically returned to the pool (if the pool still exists and is not full)
	 * or deallocated.
	 */
	using MemoryBlockSharedPtr = std::shared_ptr<std::uint8_t[]>;

	/**
	 * @brief Factory function to create a new MemoryBlockPool instance.
	 *
	 * As the constructor is private, this function must be used to create the pool.
	 *
	 * @param logger The instance of ILogger for logging purposes.
	 * @param blockSize The size (in uint8_ts) of each memory block. Must be greater than 0
	 * and a multiple of alignment.
	 * @param alignment The memory alignment (in uint8_ts) required for each block.
	 * Must be a power of two and at least alignof(std::max_align_t).
	 * Defaults to 32.
	 * @param maxSize The maximum number of idle blocks to keep in the pool.
	 * Must be greater than 0. Defaults to 32.
	 *
	 * @throw std::invalid_argument If any of the parameters (blockSize, alignment, maxSize)
	 * do not meet the specified requirements.
	 *
	 * @return A std::shared_ptr to the newly created MemoryBlockPool.
	 */
	static std::shared_ptr<MemoryBlockPool> create(const Logger::ILogger &logger, std::size_t blockSize,
						       std::size_t alignment = 32, std::size_t maxSize = 32)
	{
		if (blockSize == 0) {
			throw std::invalid_argument("blockSize must be greater than 0");
		} else if (maxSize == 0) {
			throw std::invalid_argument("maxSize must be greater than 0");
		} else if (alignment & (alignment - 1)) {
			throw std::invalid_argument("alignment must be a power of two");
		} else if (alignment < alignof(std::max_align_t)) {
			throw std::invalid_argument("alignment must be at least " +
						    std::to_string(alignof(std::max_align_t)));
		} else if (blockSize % alignment != 0) {
			throw std::invalid_argument("blockSize must be a multiple of alignment");
		}
		return std::shared_ptr<MemoryBlockPool>(new MemoryBlockPool(logger, blockSize, alignment, maxSize));
	}

	/**
	 * @brief Destroys the memory pool.
	 *
	 * All idle blocks currently held in the pool are deallocated.
	 * Any blocks that are still in use (acquired via acquire()) will be
	 * safely deallocated when their respective shared_ptr instances go out of scope,
	 * without attempting to return them to the now-destroyed pool.
	 */
	~MemoryBlockPool() noexcept {}

	/**
	 * @brief Acquires a memory block from the pool.
	 *
	 * If the pool has an idle block available, it is returned.
	 * If the pool is empty, a new block is allocated with the specified
	 * size and alignment.
	 *
	 * @return A MemoryBlockSharedPtr (a shared_ptr) managing the acquired block.
	 * When this pointer goes out of scope, the block is automatically
	 * returned to the pool or deallocated.
	 */
	MemoryBlockSharedPtr acquire()
	{
		std::unique_ptr<std::uint8_t[], AlignedDeleter> block;
		{
			std::lock_guard<std::mutex> lock(poolMutex_);
			if (pool_.empty()) {
				block = std::unique_ptr<std::uint8_t[], AlignedDeleter>(
					new (std::align_val_t(alignment_)) std::uint8_t[blockSize_],
					AlignedDeleter{blockSize_, alignment_});
				logger_.debug("Allocated new memory block of size {} bytes with alignment {} bytes",
					      blockSize_, alignment_);
			} else {
				block = std::move(pool_.back());
				pool_.pop_back();
			}
		}

		return MemoryBlockSharedPtr(block.get(), [block = std::move(block),
							  weakSelf = weak_from_this()](std::uint8_t *) mutable {
			if (auto self = weakSelf.lock()) {
				std::lock_guard<std::mutex> lock(self->poolMutex_);
				if (self->pool_.size() < self->maxSize_) {
					self->pool_.push_back(std::move(block));
				}
			}
		});
	}

	/**
	 * @brief Gets the size of the memory blocks managed by this pool.
	 * @return The size (in uint8_ts) of each block.
	 */
	std::size_t getPixelCount() const noexcept { return blockSize_; }

private:
	struct AlignedDeleter {
		std::size_t blockSize_;
		std::size_t alignment_;
		void operator()(std::uint8_t *ptr) const noexcept
		{
			delete[] ptr;
		}
	};

	MemoryBlockPool(const Logger::ILogger &logger, std::size_t blockSize, std::size_t alignment,
			std::size_t maxSize)
		: logger_(logger),
		  blockSize_(blockSize),
		  alignment_(alignment),
		  maxSize_(maxSize)
	{
	}

	const Logger::ILogger &logger_;
	std::size_t blockSize_;
	std::size_t alignment_;
	std::size_t maxSize_;
	std::vector<std::unique_ptr<std::uint8_t[], AlignedDeleter>> pool_;
	mutable std::mutex poolMutex_;
};

} // namespace TaskQueue
} // namespace KaitoTokyo
