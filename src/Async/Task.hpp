/*
 * KaitoTokyo Async Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <utility>
#include <variant>

namespace KaitoTokyo::Async {

// -----------------------------------------------------------------------------
// SymmetricTransfer
// -----------------------------------------------------------------------------

/**
 * @brief Helper for Symmetric Transfer to prevent stack overflow.
 *
 * @details
 * Instead of recursively resuming the continuation (which consumes stack space),
 * this awaitable returns the continuation handle to the caller/resumer.
 * This allows the compiler to perform a tail call optimization (symmetric transfer).
 *
 * @tparam PromiseType The promise type of the coroutine.
 */
template<typename PromiseType> struct SymmetricTransfer {
	bool await_ready() noexcept { return false; }

	std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> h) noexcept
	{
		auto &promise = h.promise();
		if (promise.continuation_) {
			return promise.continuation_;
		}
		return std::noop_coroutine();
	}

	void await_resume() noexcept {}
};

// -----------------------------------------------------------------------------
// TaskStorage
// -----------------------------------------------------------------------------

/**
 * @brief Functor to release the used flag in TaskStorage.
 */
struct TaskStorageReleaser {
	bool *usedFlagPtr;

	void operator()(void *) const
	{
		if (usedFlagPtr) {
			*usedFlagPtr = false;
		}
	}
};

/**
 * @brief A unique_ptr that manages the "used" state of a TaskStorage slot.
 */
using TaskStoragePtr = std::unique_ptr<void, TaskStorageReleaser>;

/**
 * @brief A fixed-size memory storage for a single Task.
 *
 * @details
 * This class provides a pre-allocated buffer to store a coroutine frame, avoiding dynamic heap allocation.
 * It manages a simple "used" flag to ensure only one task occupies the buffer at a time.
 *
 * @warning **LIFETIME CONSTRAINT**
 * The `TaskStorage` instance **MUST outlive** any `Task` allocated from it.
 * The `Task` holds a `TaskStoragePtr` which refers back to the `used_` flag inside this storage.
 * If the storage is destroyed while a Task is still running, it will lead to a dangling pointer access.
 *
 * @tparam Size The size of the buffer in bytes (default: 4096).
 */
template<std::size_t Size = 4096> class TaskStorage {
	alignas(std::max_align_t) char buffer_[Size];
	bool used_ = false;

public:
	TaskStorage() = default;
	~TaskStorage() = default;

	TaskStorage(const TaskStorage &) = delete;
	TaskStorage &operator=(const TaskStorage &) = delete;

	/**
	 * @brief Allocates the buffer for a task if available.
	 *
	 * @param n Required size in bytes.
	 * @return A `TaskStoragePtr` managing the buffer ownership, or an empty ptr if unavailable/too small.
	 */
	TaskStoragePtr allocate(std::size_t n)
	{
		if (n > Size || used_) {
			return TaskStoragePtr(nullptr, TaskStorageReleaser{nullptr});
		}

		used_ = true;
		return TaskStoragePtr(buffer_, TaskStorageReleaser{&used_});
	}
};

// -----------------------------------------------------------------------------
// Task
// -----------------------------------------------------------------------------

/**
 * @brief Base class for TaskPromise handling result values or exceptions.
 */
template<typename T> struct TaskPromiseBase {
	std::variant<std::monostate, T, std::exception_ptr> result_;

	void return_value(T v) { result_.template emplace<1>(std::move(v)); }

	void unhandled_exception() { result_.template emplace<2>(std::current_exception()); }

	T extract_value()
	{
		if (result_.index() == 2) {
			std::rethrow_exception(std::get<2>(result_));
		}
		return std::move(std::get<1>(result_));
	}
};

/**
 * @brief Specialization of TaskPromiseBase for void.
 */
template<> struct TaskPromiseBase<void> {
	std::variant<std::monostate, std::exception_ptr> result_;

	void return_void() {}

	void unhandled_exception() { result_.template emplace<1>(std::current_exception()); }

	void extract_value()
	{
		if (result_.index() == 1) {
			std::rethrow_exception(std::get<1>(result_));
		}
	}
};

/**
 * @brief A coroutine task with unique ownership and custom allocation support.
 *
 * @details
 * - **Unique Ownership**: Unlike `SharedTask`, this task is move-only and owns the coroutine handle.
 * - **Custom Allocation**: It uses `TaskStorage` (via `operator new` overload) to allocate the coroutine frame
 * on a provided buffer, minimizing heap usage.
 * - **Symmetric Transfer**: Uses symmetric transfer for tail-call optimization in `await_suspend`.
 *
 * @note
 * The `Task` manages the lifetime of the allocated `TaskStorage` slot by embedding a `TaskStoragePtr`
 * directly into the allocated memory block (header).
 *
 * @tparam T The return type of the task.
 */
template<typename T> struct [[nodiscard]] Task {
	struct promise_type : TaskPromiseBase<T> {
		std::coroutine_handle<> continuation_ = nullptr;

		Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }

		std::suspend_always initial_suspend() { return {}; }
		auto final_suspend() noexcept { return SymmetricTransfer<promise_type>{}; }

		/**
		 * @brief Custom operator new to allocate coroutine frame from TaskStorage.
		 * * @details
		 * It expects a `TaskStorage` compatible allocator to be passed as an argument.
		 * It allocates extra space to embed the `TaskStoragePtr` (RAII object) at the beginning of the block.
		 * The offset for the coroutine frame is calculated to ensure proper alignment (`max_align_t`).
		 */
		template<typename Alloc, typename... Args>
		static void *operator new(std::size_t size, std::allocator_arg_t, Alloc &alloc, Args &&...)
		{
#ifdef NDEBUG
			// 1. Calculate header size and alignment padding
			constexpr std::size_t alignment = alignof(std::max_align_t);
			std::size_t header_size = sizeof(TaskStoragePtr);

			// Round up header_size to the next multiple of alignment
			std::size_t offset = (header_size + alignment - 1) & ~(alignment - 1);

			std::size_t total_size = size + offset;

			// 2. Allocate memory from the provided allocator (TaskStorage)
			auto ptr = alloc.allocate(total_size);

			if (!ptr) {
				throw std::bad_alloc();
			}

			void *raw_ptr = ptr.get();

			// 3. Construct the TaskStoragePtr (RAII) in the header region
			// This keeps the storage slot "used" as long as this memory block exists.
			new (raw_ptr) TaskStoragePtr(std::move(ptr));

			// 4. Return the pointer offset by the padded header size
			return static_cast<char *>(raw_ptr) + offset;
#else
			(void)alloc;
			return ::operator new(size);
#endif
		}

		static void *operator new(std::size_t) = delete;

		static void operator delete(void *ptr, std::size_t)
		{
#ifdef NDEBUG
			// 1. Re-calculate offset to find the start of the allocated block
			constexpr std::size_t alignment = alignof(std::max_align_t);
			std::size_t header_size = sizeof(TaskStoragePtr);
			std::size_t offset = (header_size + alignment - 1) & ~(alignment - 1);

			char *raw_ptr = static_cast<char *>(ptr) - offset;

			// 2. Destruct the TaskStoragePtr to release the storage slot
			auto *stored_ptr = reinterpret_cast<TaskStoragePtr *>(raw_ptr);
			stored_ptr->~TaskStoragePtr();
#else
			::operator delete(ptr);
#endif
		}
	};

	explicit Task(std::coroutine_handle<promise_type> h) : handle_(h) {}
	~Task()
	{
		if (handle_)
			handle_.destroy();
	}

	Task(const Task &) = delete;
	Task &operator=(const Task &) = delete;
	Task(Task &&other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
	Task &operator=(Task &&other) noexcept
	{
		if (this != &other) {
			if (handle_)
				handle_.destroy();
			handle_ = other.handle_;
			other.handle_ = nullptr;
		}
		return *this;
	}

	bool await_ready() { return handle_.done(); }

	std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller)
	{
		handle_.promise().continuation_ = caller;
		return handle_;
	}

	T await_resume() { return handle_.promise().extract_value(); }

	void start()
	{
		if (handle_ && !handle_.done())
			handle_.resume();
	}

private:
	std::coroutine_handle<promise_type> handle_;
};

} // namespace KaitoTokyo::Async
