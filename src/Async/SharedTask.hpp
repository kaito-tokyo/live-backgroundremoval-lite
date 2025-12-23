/*
 * KaitoTokyo Async Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <utility>
#include <variant>

namespace KaitoTokyo::Async {

#ifdef NDEBUG
constexpr std::size_t kDefaultSharedTaskSize = 4096;
#else
constexpr std::size_t kDefaultSharedTaskSize = 32768;
#endif

// -----------------------------------------------------------------------------
// Common Definitions
// -----------------------------------------------------------------------------
struct SharedTaskAwaiterNode {
	std::coroutine_handle<> continuation;
	SharedTaskAwaiterNode *next = nullptr;
};

// -----------------------------------------------------------------------------
// 1. Standard Implementation: SharedTaskContext<T>
// -----------------------------------------------------------------------------

/**
 * @brief Manages the execution context (state, result, and waiters list) of a shared task.
 *
 * @details
 * This class holds the result of the task (value or exception) and a list of coroutines waiting for completion.
 * It also embeds a memory buffer to store the coroutine frame, minimizing heap allocations.
 *
 * @note
 * This object is typically managed by `std::shared_ptr`.
 * Since the corresponding `SharedTask` only holds a `std::weak_ptr` to this context,
 * **the user is responsible for maintaining the `std::shared_ptr` to this context** while the task is active or being awaited.
 *
 * @tparam T The type of the result produced by the task.
 * @tparam Size The size (in bytes) of the internal buffer allocated for the coroutine frame. This is a non-type template parameter.
 */
template<typename T, std::size_t Size = kDefaultSharedTaskSize> struct SharedTaskContext {
	using value_type = T;

	// --- State ---
	std::variant<std::monostate, T, std::exception_ptr> result;
	std::atomic<SharedTaskAwaiterNode *> waiters_head{nullptr};
	static inline SharedTaskAwaiterNode kFinishedNode = {};

	// --- Coroutine Management ---
	std::coroutine_handle<> handle = nullptr;

	// --- Memory Buffer ---
	alignas(std::max_align_t) char buffer[Size];
	bool used = false;

	SharedTaskContext() = default;

	~SharedTaskContext()
	{
		if (handle)
			handle.destroy();
	}

	SharedTaskContext(const SharedTaskContext &) = delete;
	SharedTaskContext &operator=(const SharedTaskContext &) = delete;
	SharedTaskContext(SharedTaskContext &&) = delete;
	SharedTaskContext &operator=(SharedTaskContext &&) = delete;

	void return_value(T &&v) { result.template emplace<1>(std::move(v)); }
	void unhandled_exception() { result.template emplace<2>(std::current_exception()); }

	bool try_await(SharedTaskAwaiterNode *node)
	{
		auto *old_head = waiters_head.load(std::memory_order_acquire);
		do {
			if (old_head == &kFinishedNode)
				return false;
			node->next = old_head;
		} while (!waiters_head.compare_exchange_weak(old_head, node, std::memory_order_release,
							     std::memory_order_acquire));
		return true;
	}

	T get_result()
	{
		if (result.index() == 2)
			std::rethrow_exception(std::get<2>(result));
		return std::get<1>(result);
	}

	void notify_waiters()
	{
		auto *curr = waiters_head.exchange(&kFinishedNode, std::memory_order_acq_rel);
		while (curr) {
			auto *next = curr->next;
			if (curr->continuation)
				curr->continuation.resume();
			curr = next;
		}
	}

	bool is_ready() const { return waiters_head.load(std::memory_order_acquire) == &kFinishedNode; }

	void *allocate_frame(std::size_t size)
	{
		if (size > Size || used)
			throw std::bad_alloc();
		used = true;
		return buffer;
	}
};

// Specialization for void
template<std::size_t Size> struct SharedTaskContext<void, Size> {
	using value_type = void;
	std::variant<std::monostate, std::monostate, std::exception_ptr> result;
	std::atomic<SharedTaskAwaiterNode *> waiters_head{nullptr};
	static inline SharedTaskAwaiterNode kFinishedNode = {};
	std::coroutine_handle<> handle = nullptr;
	alignas(std::max_align_t) char buffer[Size];
	bool used = false;

	SharedTaskContext() = default;
	~SharedTaskContext()
	{
		if (handle)
			handle.destroy();
	}
	SharedTaskContext(const SharedTaskContext &) = delete;
	SharedTaskContext &operator=(const SharedTaskContext &) = delete;
	SharedTaskContext(SharedTaskContext &&) = delete;
	SharedTaskContext &operator=(SharedTaskContext &&) = delete;

	void return_void() { result.template emplace<1>(std::monostate{}); }
	void unhandled_exception() { result.template emplace<2>(std::current_exception()); }

	bool try_await(SharedTaskAwaiterNode *node)
	{
		auto *old_head = waiters_head.load(std::memory_order_acquire);
		do {
			if (old_head == &kFinishedNode)
				return false;
			node->next = old_head;
		} while (!waiters_head.compare_exchange_weak(old_head, node, std::memory_order_release,
							     std::memory_order_acquire));
		return true;
	}

	void get_result()
	{
		if (result.index() == 2)
			std::rethrow_exception(std::get<2>(result));
	}

	void notify_waiters()
	{
		auto *curr = waiters_head.exchange(&kFinishedNode, std::memory_order_acq_rel);
		while (curr) {
			auto *next = curr->next;
			if (curr->continuation)
				curr->continuation.resume();
			curr = next;
		}
	}

	bool is_ready() const { return waiters_head.load(std::memory_order_acquire) == &kFinishedNode; }

	void *allocate_frame(std::size_t size)
	{
		if (size > Size || used)
			throw std::bad_alloc();
		used = true;
		return buffer;
	}
};

// -----------------------------------------------------------------------------
// Forward Declaration: Promise
// -----------------------------------------------------------------------------
template<typename T, typename Context> struct SharedTaskPromise;

// -----------------------------------------------------------------------------
// 2. SharedTask (General Handle)
// -----------------------------------------------------------------------------

/**
 * @brief An asynchronous task handle that allows multiple waiters to share the result.
 *
 * @details
 * Similar to `std::shared_future`, this task can be `co_await`ed from multiple places.
 *
 * @warning **IMPORTANT: LIFETIME CONTRACT**
 * This class holds only a **weak reference (std::weak_ptr)** to the `SharedTaskContext`.
 * While this design prevents circular references, it imposes strict lifetime management responsibilities on the user:
 *
 * 1. **Ownership Maintenance**:
 * Holding this `SharedTask` instance alone **does NOT keep the task state (Context) alive**.
 * The caller (user) MUST strictly maintain the ownership of the `std::shared_ptr<SharedTaskContext<T>>`
 * for as long as the task is running or needs to be awaited.
 *
 * 2. **Behavior on Context Destruction**:
 * If the `SharedTaskContext` is destroyed before the task is awaited, attempting to `co_await`
 * this task will fail to lock the `weak_context` and result in an immediate **`std::terminate()`**.
 *
 * Example Usage:
 * @code
 * auto ctx = std::make_shared<SharedTaskContext<int>>();
 * SharedTask<int> task = my_coroutine(std::allocator_arg, ctx);
 *
 * // The 'task' is valid only while 'ctx' is held.
 * co_await task;
 *
 * // ctx.reset(); // If 'ctx' is released here, 'task' becomes unusable.
 * @endcode
 *
 * @tparam T The type of the value returned by the task.
 * @tparam Context The context type (default is `SharedTaskContext<T>`).
 */
template<typename T, typename Context = SharedTaskContext<T>> struct SharedTask {
	using promise_type = SharedTaskPromise<T, Context>;

	std::weak_ptr<Context> weak_context;

	SharedTask() = default;

	explicit SharedTask(const std::shared_ptr<Context> &context) : weak_context(context) {}

	struct Awaiter {
		std::shared_ptr<Context> context;
		SharedTaskAwaiterNode node;

		bool await_ready() { return context->is_ready(); }

		bool await_suspend(std::coroutine_handle<> caller)
		{
			node.continuation = caller;
			return context->try_await(&node);
		}

		T await_resume() { return context->get_result(); }
	};

	Awaiter operator co_await() const
	{
		if (auto context = weak_context.lock()) {
			return Awaiter{std::move(context)};
		} else {
			// If the context has already been destroyed, terminate as an unrecoverable error.
			std::terminate();
		}
	}
};

// -----------------------------------------------------------------------------
// SharedTaskPromiseBase: Handles return_value / return_void differentiation
// -----------------------------------------------------------------------------

// 1. Primary Template (T != void)
template<typename T, typename Context> struct SharedTaskPromiseBase {
	std::weak_ptr<Context> weak_context;

	void return_value(const T &v)
	{
		if (auto context = weak_context.lock()) {
			context->return_value(v);
		} else {
			std::terminate();
		}
	}

	void return_value(T &&v)
	{
		if (auto context = weak_context.lock()) {
			context->return_value(std::move(v));
		} else {
			std::terminate();
		}
	}

	void unhandled_exception()
	{
		if (auto context = weak_context.lock()) {
			context->unhandled_exception();
		} else {
			std::terminate();
		}
	}
};

// 2. Specialization for void (T == void)
template<typename Context> struct SharedTaskPromiseBase<void, Context> {
	std::weak_ptr<Context> weak_context;

	void return_void()
	{
		if (auto context = weak_context.lock()) {
			context->return_void();
		} else {
			std::terminate();
		}
	}

	void unhandled_exception()
	{
		if (auto context = weak_context.lock()) {
			context->unhandled_exception();
		} else {
			std::terminate();
		}
	}
};

// -----------------------------------------------------------------------------
// 3. SharedTaskPromise (Inherits from Base and implements promise requirements)
// -----------------------------------------------------------------------------
template<typename T, typename Context> struct SharedTaskPromise : SharedTaskPromiseBase<T, Context> {

	template<typename... Args>
	SharedTaskPromise(std::allocator_arg_t, const std::shared_ptr<Context> &context, Args &&...)
	{
		this->weak_context = context;
	}

	SharedTask<T, Context> get_return_object()
	{
		auto h = std::coroutine_handle<SharedTaskPromise>::from_promise(*this);
		if (auto strong_context = this->weak_context.lock()) {
			strong_context->handle = h;
			return SharedTask<T, Context>{strong_context};
		} else {
			std::terminate();
		}
	}

	std::suspend_never initial_suspend() { return {}; }

	struct FinalAwaiter {
		bool await_ready() noexcept { return false; }

		std::coroutine_handle<> await_suspend(std::coroutine_handle<SharedTaskPromise> h) noexcept
		{
			h.promise().weak_context.lock()->notify_waiters();
			return std::noop_coroutine();
		}
		void await_resume() noexcept {}
	};

	FinalAwaiter final_suspend() noexcept { return {}; }

	template<typename... Args>
	static void *operator new(std::size_t size, std::allocator_arg_t, const std::shared_ptr<Context> &context,
				  Args &&...)
	{
		return context->allocate_frame(size);
	}

	static void operator delete(void *, std::size_t) {}
};

} // namespace KaitoTokyo::Async
