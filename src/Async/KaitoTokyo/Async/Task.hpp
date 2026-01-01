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

/**
 * =============================================================================
 * KAITOTOKYO ASYNC LIBRARY - GENERAL PURPOSE TASK
 * =============================================================================
 *
 * @brief High-Performance, General-Purpose C++20 Coroutine Task
 *
 * This header defines `Task<T>`, the standard unit of asynchronous work for
 * this library. It is designed to be the default choice for general usage,
 * balancing ease of use with maximum runtime performance.
 *
 * @section PERFORMANCE Performance Characteristics
 *
 * 1. **Optimized Execution**: Beyond the initial memory allocation, the execution
 * model is zero-overhead. It utilizes **Symmetric Transfer** to switch contexts,
 * preventing stack overflows during deep recursion and minimizing CPU cycles.
 * 2. **Standard Allocation**: This implementation uses standard heap allocation
 * (`operator new`) for the coroutine frame. This ensures compatibility with
 * standard C++ memory models.
 *
 * @section LIFECYCLE Ownership & Destruction
 *
 * This library enforces strict **unique ownership** (RAII):
 *
 * 1. **Resource Management**: The `Task` object owns the coroutine frame.
 * 2. **Destruction**: When the `Task` object is destroyed (e.g., goes out of scope
 * or is assigned `Task{}`), the coroutine frame is immediately deallocated.
 * 3. **Effect of Destruction**:
 * - **If Suspended**: The coroutine is effectively cancelled because it can
 * no longer be resumed. Local variables in the coroutine are destructed.
 * - **If Running**: DO NOT destroy the `Task` object while the coroutine is
 * executing instructions (e.g., on another thread). This causes a
 * Use-After-Free error.
 *
 * @section EXAMPLE Usage
 *
 * @code
 * // A standard async function
 * Task<int> calculate_async(int value) {
 *         co_return value * 2;
 * }
 *
 * int main() {
 *         Task<int> task; // Default constructible (empty state)
 *         task = calculate_async(10); // Move assignment
 *         task.start(); // Kick off execution manually
 *
 *         return 0;
 * }
 * @endcode
 * =============================================================================
 */

#include <coroutine>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

namespace KaitoTokyo::Async {

// -----------------------------------------------------------------------------
// SymmetricTransfer
// Implements symmetric control transfer to allow tail-call optimization
// in the coroutine state machine, preventing stack overflow in deep chains.
// -----------------------------------------------------------------------------

template<typename PromiseType> struct TaskSymmetricTransfer {
	bool await_ready() noexcept { return false; }

	std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> h) noexcept
	{
		auto &promise = h.promise();
		if (promise.continuation) {
			return promise.continuation;
		}
		return std::noop_coroutine();
	}

	void await_resume() noexcept {}
};

// -----------------------------------------------------------------------------
// Promise Types
// -----------------------------------------------------------------------------

template<typename T> struct TaskPromiseBase {
	// Index 0: std::monostate (Running/Not Ready)
	// Index 1: T (Result Value)
	// Index 2: std::exception_ptr (Error)
	std::variant<std::monostate, T, std::exception_ptr> result_;

	void return_value(T v) { result_.template emplace<1>(std::move(v)); }
	void unhandled_exception() { result_.template emplace<2>(std::current_exception()); }

	T extract_value()
	{
		if (result_.index() == 2) {
			std::rethrow_exception(std::get<2>(result_));
		} else if (result_.index() == 0) {
			throw std::logic_error("Task result is not ready. Do not await a running task.");
		}
		return std::move(std::get<1>(result_));
	}
};

template<> struct TaskPromiseBase<void> {
	// Index 0: std::monostate (Running/Not Ready)
	// Index 1: std::monostate (Done/Void Result)
	// Index 2: std::exception_ptr (Error)
	std::variant<std::monostate, std::monostate, std::exception_ptr> result_;

	TaskPromiseBase() : result_(std::in_place_index<0>) {}

	void return_void() { result_.template emplace<1>(); }
	void unhandled_exception() { result_.template emplace<2>(std::current_exception()); }

	void extract_value()
	{
		if (result_.index() == 2) {
			std::rethrow_exception(std::get<2>(result_));
		} else if (result_.index() == 0) {
			throw std::logic_error("Task result is not ready. Do not await a running task.");
		}
	}
};

/**
 * @brief The standard coroutine task for asynchronous operations.
 *
 * @details
 * This task is lazy (does not start until awaited or explicitly started) and
 * strictly manages the lifecycle of the coroutine frame via RAII.
 *
 * It is optimized for runtime performance using symmetric transfer mechanics,
 * making it suitable for high-frequency async operations where standard
 * allocation overhead is acceptable.
 */
template<typename T>
struct [[nodiscard("Task objects own the running coroutine. Do not discard without awaiting or storing.")]] Task {
	struct promise_type : TaskPromiseBase<T> {
		std::coroutine_handle<> continuation = nullptr;

		Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }

		std::suspend_always initial_suspend() { return {}; }
		auto final_suspend() noexcept { return TaskSymmetricTransfer<promise_type>{}; }
	};

	// Default constructor creates an empty task.
	Task() noexcept : handle(nullptr) {}

	explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}

	// Destructor: Automatically destroys the coroutine frame (cancellation).
	~Task() noexcept
	{
		if (handle)
			handle.destroy();
	}

	// Disable copying to enforce unique ownership.
	Task(const Task &) = delete;
	Task &operator=(const Task &) = delete;

	// Enable moving to transfer ownership.
	Task(Task &&other) noexcept : handle(other.handle) { other.handle = nullptr; }
	Task &operator=(Task &&other) noexcept
	{
		if (this != &other) {
			if (handle)
				handle.destroy();
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}

	/**
	 * @brief Checks if the task object currently owns a valid coroutine.
	 * @return true if the task is valid, false if it is empty.
	 */
	explicit operator bool() const noexcept { return handle != nullptr; }

	[[nodiscard]] bool await_ready() const noexcept { return !handle || handle.done(); }

	std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller)
	{
		handle.promise().continuation = caller;
		return handle;
	}

	T await_resume() { return handle.promise().extract_value(); }

	/**
	 * @brief Explicitly starts the task.
	 *
	 * Use this method when the task is the root of an execution chain
	 * (e.g., called from main() or a synchronous event handler).
	 * If the task is being awaited by another coroutine, this call is not needed.
	 * @note Calling start() on an empty task is a no-op.
	 */
	void start()
	{
		if (handle && !handle.done())
			handle.resume();
	}

private:
	std::coroutine_handle<promise_type> handle;
};

} // namespace KaitoTokyo::Async
