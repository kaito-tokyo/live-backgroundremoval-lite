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

/**
 * =============================================================================
 * KAITOTOKYO ASYNC LIBRARY - INTERNAL API DOCUMENTATION
 * =============================================================================
 *
 * @section WARNING CRITICAL USAGE WARNING
 *
 * This library provides a zero-overhead, allocation-aware coroutine mechanism
 * designed for precise memory control. Unlike general-purpose async libraries,
 * it DOES NOT manage lifecycles automatically via reference counting.
 *
 * @brief THE "POWER USER" CONTRACT
 *
 * By using this library, you agree to the following STRICT contracts.
 * Failure to adhere to these rules will result in `std::terminate`, memory corruption,
 * or undefined behavior.
 *
 * 1. **EXPLICIT OWNERSHIP**: The `Task` object uniquely owns the coroutine frame.
 * 2. **NO FIRE-AND-FORGET**: If a `Task` object goes out of scope, the coroutine
 * is immediately destroyed (cancelled). You MUST keep the `Task` object alive
 * until the operation completes.
 * 3. **STRICT STORAGE HIERARCHY**: When using `TaskStorage`, the storage object
 * MUST strictly outlive the `Task` created from it.
 * 4. **ALLOCATOR ARGUMENT REQUIREMENT**:
 * To enable the custom allocation mechanism, every coroutine function returning
 * a `Task` MUST accept `std::allocator_arg_t` as the first argument and
 * a reference to a `TaskAllocator` (e.g., `TaskStorage&`) as the second argument.
 * Failure to do so will result in a compilation error regarding `operator new`.
 *
 * Example:
 * @code
 * Task<int> my_coroutine(std::allocator_arg_t, TaskStorage<>& storage, int arg1) {
 * co_return arg1 * 2;
 * }
 * @endcode
 *
 * @section STACK_USAGE STACK CONSUMPTION WARNING
 *
 * `TaskStorage` reserves a fixed memory block directly inline (Default: 4KB in Release,
 * 32KB in Debug).
 *
 * - **Avoid Recursive Stack Allocation**: Instantiating `TaskStorage` as a local
 * variable (automatic storage duration) inside a recursive function will rapidly
 * cause a Stack Overflow due to the cumulative size.
 * - **Placement Strategy**: Users are strongly encouraged to use `static`,
 * `thread_local`, or external lifetime management (e.g., as a member of a
 * heap-allocated object) rather than placing storage on the transient call stack
 * unless the recursion depth is strictly bounded.
 *
 * @section THREAD_SAFETY THREAD SAFETY AND EXECUTION MODEL
 *
 * - **Cross-Thread Execution**: It is explicitly permitted for the thread that calls
 * `Task::start()` to be different from the thread that `co_await`s the result.
 * The library is designed to allow the task to migrate or be awaited across
 * thread boundaries, provided that ownership of the `Task` object is correctly managed.
 * - **Synchronization**: While the `Task` object itself manages the lifecycle,
 * users must ensure appropriate memory visibility when accessing shared state
 * outside the task's return value.
 *
 * Use this library only when standard dynamic allocation is unacceptable and
 * you can guarantee the lifetime hierarchy of your objects.
 * =============================================================================
 */

#include <atomic>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace KaitoTokyo::Async {

#ifdef NDEBUG
constexpr std::size_t kDefaultTaskSize = 4096;
#else
constexpr std::size_t kDefaultTaskSize = 32768;
#endif

// -----------------------------------------------------------------------------
// SymmetricTransfer
// -----------------------------------------------------------------------------

template<typename PromiseType> struct SymmetricTransfer {
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
// TaskStorage
// -----------------------------------------------------------------------------

struct TaskStorageReleaser {
	using CheckerFunc = void (*)(void *storage_ptr);

	void *storage_ptr;
	CheckerFunc checker;

	void operator()(void *) const
	{
		if (storage_ptr && checker) {
			checker(storage_ptr);
		}
	}
};

using TaskStoragePtr = std::unique_ptr<void, TaskStorageReleaser>;

/**
 * @brief A fixed-size memory storage block for a single Task.
 *
 * @details
 * This class provides deterministic memory placement for a coroutine frame.
 * It is intended for scenarios where heap allocation is too slow or non-deterministic.
 *
 * @warning **CRITICAL LIFETIME CONSTRAINT**
 * The `TaskStorage` instance MUST outlive any `Task` (coroutine) allocated from it.
 *
 * @note **FATAL ERROR BEHAVIOR**
 * If the `TaskStorage` destructor is called while the memory is still marked as
 * `used_` (i.e., the associated Task is still alive/running), the program
 * will immediately call `std::terminate()`.
 */
template<std::size_t Size = kDefaultTaskSize> class TaskStorage {
	enum class State : std::uint64_t { Alive = 0x5441534B4C495645, Dead = 0xDEADDEADDEADDEAD };

	State magic_ = State::Alive;
	std::atomic<bool> used_{false};
	alignas(std::max_align_t) std::byte buffer_[Size];

public:
	TaskStorage() = default;

	// Ensure storage is not destroyed while a task is running
	~TaskStorage()
	{
		if (used_.load(std::memory_order_acquire)) {
			// FATAL: Storage destroyed while Task is still running.
			std::terminate();
		}
		magic_ = State::Dead;
	};

	TaskStorage(const TaskStorage &) = delete;
	TaskStorage &operator=(const TaskStorage &) = delete;
	TaskStorage(TaskStorage &&) = delete;
	TaskStorage &operator=(TaskStorage &&) = delete;

	[[nodiscard("Ignoring allocate() result causes immediate deallocation")]]
	TaskStoragePtr allocate(std::size_t n)
	{
		if (n > Size) {
			used_.store(false, std::memory_order_release);
			throw std::length_error("InsufficientCapacityError(TaskStorage::allocate):" + std::to_string(n) + "/" + std::to_string(Size));
		}

		bool expected = false;
		if (!used_.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
			throw std::logic_error("IllegalReuseError(TaskStorage::allocate)");
		}

		// This checker will be converted into a function pointer to avoid capturing anything.
		auto checker = [](void *ptr) {
			auto *self = static_cast<TaskStorage *>(ptr);
			if (self->magic_ != State::Alive) {
				// FATAL: Use-after-free detected. Storage was destroyed before the Task.
				std::terminate();
			}
			self->used_.store(false, std::memory_order_release);
		};

		return TaskStoragePtr(buffer_, TaskStorageReleaser{this, checker});
	}
};

// -----------------------------------------------------------------------------
// Concepts
// -----------------------------------------------------------------------------

template<typename T> concept TaskAllocator = requires(T & a, std::size_t n)
{
	{
		a.allocate(n)
	} -> std::convertible_to<TaskStoragePtr>;
};

// -----------------------------------------------------------------------------
// Task
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
	// Index 1: std::monostate (Done/Void Result) - used_ as a flag for completion
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
		// Index 1 means success (void), simply return.
	}
};

/**
 * @brief A unique-ownership coroutine task.
 *
 * @details
 * This class implements a strict RAII (Resource Acquisition Is Initialization) model
 * for coroutines. It does not support shared ownership or detachment.
 *
 * @warning **FIRE-AND-FORGET PROHIBITED**
 * You MUST store the returned `Task` object. If the `Task` object is destroyed
 * (e.g., goes out of scope) while the coroutine is suspended, the coroutine
 * frame is immediately destroyed and execution is cancelled.
 *
 * @warning **NO AUTOMATIC START**
 * Tasks are lazy. They do not start executing until you explicitly call `.start()`
 * or `co_await` them.
 */
template<typename T>
struct [[nodiscard("PROHIBITED: Fire-and-Forget. You must store this Task object to keep the coroutine alive.")]] Task {
	struct promise_type : TaskPromiseBase<T> {
		std::coroutine_handle<> continuation = nullptr;

		Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }

		std::suspend_always initial_suspend() { return {}; }
		auto final_suspend() noexcept { return SymmetricTransfer<promise_type>{}; }

		template<TaskAllocator Alloc, typename... Args>
		static void *operator new(std::size_t size, std::allocator_arg_t, Alloc &alloc, Args &&...)
		{
			constexpr std::size_t alignment = alignof(std::max_align_t);
			std::size_t header_size = sizeof(TaskStoragePtr);
			std::size_t offset = (header_size + alignment - 1) & ~(alignment - 1);
			std::size_t total_size = size + offset;

			auto ptr = alloc.allocate(total_size);

			if (!ptr) {
				throw std::logic_error("AllocateError(Task::promise_type::operator new)");
			}

			void *raw_ptr = ptr.get();
			new (raw_ptr) TaskStoragePtr(std::move(ptr));
			return static_cast<std::byte *>(raw_ptr) + offset;
		}

		static void *operator new(std::size_t) = delete;

		static void operator delete(void *ptr, std::size_t)
		{
			constexpr std::size_t alignment = alignof(std::max_align_t);
			std::size_t header_size = sizeof(TaskStoragePtr);
			std::size_t offset = (header_size + alignment - 1) & ~(alignment - 1);

			std::byte *raw_ptr = static_cast<std::byte *>(ptr) - offset;
			auto *stored_ptr = reinterpret_cast<TaskStoragePtr *>(raw_ptr);
			stored_ptr->~TaskStoragePtr();
		}
	};

	explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}

	// Destructor enforces strict lifecycle management.
	~Task()
	{
		if (handle)
			handle.destroy();
	}

	Task(const Task &) = delete;
	Task &operator=(const Task &) = delete;
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

	[[nodiscard]] bool await_ready() { return handle.done(); }

	std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller)
	{
		handle.promise().continuation = caller;
		return handle;
	}

	T await_resume() { return handle.promise().extract_value(); }

	/**
	 * @brief Kicks off the root task execution.
	 *
	 * @warning **LIFETIME HAZARD**
	 * Calling `start()` DOES NOT detach the task. You must continue to hold the
	 * `Task` object after calling `start()`. If the `Task` object is destroyed
	 * immediately after `start()`, the coroutine will be aborted before it finishes.
	 *
	 * @warning **SINGLE EXECUTION ONLY**
	 * This method must be called exactly once. Calling `start()` on a task that
	 * has already been started (whether running or suspended) is undefined behavior.
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
