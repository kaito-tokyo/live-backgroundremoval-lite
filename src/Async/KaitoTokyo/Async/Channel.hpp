/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo Async Library
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

#include <coroutine>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace KaitoTokyo::Async {

/**
 * @brief Constraint for types that can be sent through the Channel.
 *
 * To ensure exception safety during asynchronous message passing, the message type `T`
 * must be move-constructible, and the move operation must be `noexcept`.
 * This prevents data loss or corruption if an exception were to occur while
 * moving a message out of the channel.
 */
template<typename T> concept ChannelMessage = std::movable<T> && std::is_nothrow_move_constructible_v<T>;

/**
 * @brief A high-performance, thread-safe asynchronous MPSC Channel.
 *
 * @details
 * This class implements a **Multiple Producer, Single Consumer (MPSC)** queue
 * designed for C++20 coroutines. It allows multiple threads (or coroutines) to
 * send data safely, while a single consumer coroutine receives and processes them.
 *
 * Key Features:
 * - **MPSC Design**: Safe for concurrent calls to `send()`, but requires strictly
 * one active awaiter on `receive()` at a time.
 * - **Zero-Overhead Suspension**: Uses `std::coroutine_handle` to suspend the
 * receiver directly without blocking OS threads.
 * - **Graceful Shutdown**: Supports a `close()` operation that allows the consumer
 * to drain remaining items before terminating.
 * - **Exception Safety**: Enforces `noexcept` move semantics via `ChannelMessage`.
 *
 * @tparam T The type of message to transport. Must satisfy `ChannelMessage`.
 */
template<ChannelMessage T> class Channel {
public:
	Channel() = default;

	~Channel() noexcept
	{
		std::unique_lock lock(mutex_, std::defer_lock);

		std::coroutine_handle<> h = nullptr;

		try {
			lock.lock();

			if (!closed_) {
				closed_ = true;
				if (receiver_) {
					h = receiver_;
					receiver_ = nullptr;
				}
			}
		} catch (...) {
			return;
		}

		if (h) {
			h.destroy();
		}
	}

	// Non-copyable and Non-movable to ensure unique ownership and stable address for the mutex.
	Channel(const Channel &) = delete;
	Channel &operator=(const Channel &) = delete;
	Channel(Channel &&) = delete;
	Channel &operator=(Channel &&) = delete;

	/**
	 * @brief Closes the channel for new submissions.
	 *
	 * Once closed, no new items can be sent. However, existing items in the queue
	 * remain accessible to the receiver.
	 *
	 * - If a receiver is waiting (suspended), it is resumed immediately.
	 * - Subsequent calls to `receive()` will continue to yield items until the queue is empty.
	 * - Once empty, `receive()` will return `std::nullopt`.
	 */
	void close()
	{
		std::coroutine_handle<> h = nullptr;
		{
			std::scoped_lock lock(mutex_);

			if (closed_)
				return;

			closed_ = true;

			// If a receiver is waiting, grab its handle to resume it outside the lock.
			if (receiver_) {
				h = receiver_;
				receiver_ = nullptr;
			}
		}

		if (h) {
			h.resume();
		}
	}

	/**
	 * @brief Sends a value to the channel.
	 *
	 * This method is thread-safe and can be called concurrently by multiple producers.
	 *
	 * @param value The item to send. It will be moved into the internal queue.
	 * @return `true` if the item was successfully queued.
	 * @return `false` if the channel is already closed.
	 */
	bool send(T value)
	{
		std::coroutine_handle<> h = nullptr;
		{
			std::scoped_lock lock(mutex_);
			if (closed_)
				return false; // Cannot send to a closed channel.

			queue_.push_back(std::move(value));

			// If there is a waiting receiver, wake it up.
			if (receiver_) {
				h = receiver_;
				receiver_ = nullptr;
			}
		}

		// Resume outside the lock to minimize contention.
		if (h) {
			h.resume();
		}

		return true;
	}

	/**
	 * @brief Asynchronously receives a value from the channel.
	 *
	 * @warning This method follows the **Single Consumer** contract. It must NOT be
	 * awaited concurrently by multiple coroutines. Doing so results in undefined behavior.
	 *
	 * @return An awaitable object that yields `std::optional<T>`.
	 * - Returns `T` (wrapped in optional) if data is available.
	 * - Returns `std::nullopt` if the channel is closed and empty.
	 */
	[[nodiscard("You must co_await the received value.")]]
	auto receive()
	{
		struct ReceiveAwaiter {
			Channel &ch;

			// Always check condition in await_suspend to handle race conditions under lock.
			bool await_ready() const noexcept { return false; }

			bool await_suspend(std::coroutine_handle<> h)
			{
				std::scoped_lock lock(ch.mutex_);

				// Case 1: Data is available OR Channel is closed.
				// We should not suspend; proceed immediately to await_resume.
				if (!ch.queue_.empty() || ch.closed_) {
					return false;
				}

				// Case 2: Queue is empty and Channel is open.
				// Register the handle and suspend execution.
				ch.receiver_ = h;
				return true;
			}

			std::optional<T> await_resume()
			{
				std::scoped_lock lock(ch.mutex_);

				// Priority 1: Drain the queue.
				// Even if closed, we return existing data first (Graceful Shutdown).
				if (!ch.queue_.empty()) {
					T val = std::move(ch.queue_.front());
					ch.queue_.pop_front();
					return val;
				}

				// Priority 2: Signal termination.
				// If queue is empty and closed, return nullopt.
				if (ch.closed_) {
					return std::nullopt;
				}

				// Should strictly not be reached if Single Consumer contract is obeyed,
				// but return nullopt for safety.
				return std::nullopt;
			}
		};
		return ReceiveAwaiter{*this};
	}

private:
	mutable std::mutex mutex_;
	std::deque<T> queue_;
	std::coroutine_handle<> receiver_ = nullptr;
	bool closed_ = false;
};

} // namespace KaitoTokyo::Async
