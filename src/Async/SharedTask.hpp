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
#include <exception>
#include <memory>
#include <new>
#include <utility>
#include <variant>

namespace KaitoTokyo::Async {

// -----------------------------------------------------------------------------
// 共通定義
// -----------------------------------------------------------------------------
struct SharedTaskAwaiterNode {
	std::coroutine_handle<> continuation;
	SharedTaskAwaiterNode* next = nullptr;
};

// -----------------------------------------------------------------------------
// 1. 標準実装: SharedTaskContext<T>
//	(変更なし)
// -----------------------------------------------------------------------------
template<typename T, std::size_t MemorySize = 4096>
struct SharedTaskContext {
	using value_type = T;

	// --- 状態 ---
	std::variant<std::monostate, T, std::exception_ptr> result;
	std::atomic<SharedTaskAwaiterNode*> waiters_head{nullptr};
	static inline SharedTaskAwaiterNode kFinishedNode = {};

	// --- コルーチン管理 ---
	std::coroutine_handle<> handle = nullptr;

	// --- メモリバッファ ---
	alignas(std::max_align_t) char buffer[MemorySize];
	bool used = false;

	SharedTaskContext() = default;

	~SharedTaskContext() {
		if (handle) handle.destroy();
	}

	SharedTaskContext(const SharedTaskContext&) = delete;
	SharedTaskContext& operator=(const SharedTaskContext&) = delete;
	SharedTaskContext(SharedTaskContext&&) = delete;
	SharedTaskContext& operator=(SharedTaskContext&&) = delete;

	void return_value(T&& v) { result.template emplace<1>(std::move(v)); }
	void unhandled_exception() { result.template emplace<2>(std::current_exception()); }

	bool try_await(SharedTaskAwaiterNode* node) {
		auto* old_head = waiters_head.load(std::memory_order_acquire);
		do {
			if (old_head == &kFinishedNode) return false;
			node->next = old_head;
		} while (!waiters_head.compare_exchange_weak(
			old_head, node, std::memory_order_release, std::memory_order_acquire));
		return true;
	}

	T get_result() {
		if (result.index() == 2) std::rethrow_exception(std::get<2>(result));
		return std::get<1>(result);
	}

	void notify_waiters() {
		auto* curr = waiters_head.exchange(&kFinishedNode, std::memory_order_acq_rel);
		while (curr) {
			auto* next = curr->next;
			if (curr->continuation) curr->continuation.resume();
			curr = next;
		}
	}

	bool is_ready() const {
		return waiters_head.load(std::memory_order_acquire) == &kFinishedNode;
	}

	void* allocate_frame(std::size_t size) {
		if (size > MemorySize || used) throw std::bad_alloc();
		used = true;
		return buffer;
	}
};

// void特化
template<std::size_t MemorySize>
struct SharedTaskContext<void, MemorySize> {
	using value_type = void;
	std::variant<std::monostate, std::monostate, std::exception_ptr> result;
	std::atomic<SharedTaskAwaiterNode*> waiters_head{nullptr};
	static inline SharedTaskAwaiterNode kFinishedNode = {};
	std::coroutine_handle<> handle = nullptr;
	alignas(std::max_align_t) char buffer[MemorySize];
	bool used = false;

	SharedTaskContext() = default;
	~SharedTaskContext() { if (handle) handle.destroy(); }
	SharedTaskContext(const SharedTaskContext&) = delete;
	SharedTaskContext& operator=(const SharedTaskContext&) = delete;
	SharedTaskContext(SharedTaskContext&&) = delete;
	SharedTaskContext& operator=(SharedTaskContext&&) = delete;

	void return_void() { result.template emplace<1>(std::monostate{}); }
	void unhandled_exception() { result.template emplace<2>(std::current_exception()); }

	bool try_await(SharedTaskAwaiterNode* node) {
		auto* old_head = waiters_head.load(std::memory_order_acquire);
		do {
			if (old_head == &kFinishedNode) return false;
			node->next = old_head;
		} while (!waiters_head.compare_exchange_weak(old_head, node, std::memory_order_release, std::memory_order_acquire));
		return true;
	}

	void get_result() {
		if (result.index() == 2) std::rethrow_exception(std::get<2>(result));
	}

	void notify_waiters() {
		auto* curr = waiters_head.exchange(&kFinishedNode, std::memory_order_acq_rel);
		while (curr) {
			auto* next = curr->next;
			if (curr->continuation) curr->continuation.resume();
			curr = next;
		}
	}

	bool is_ready() const { return waiters_head.load(std::memory_order_acquire) == &kFinishedNode; }

	void* allocate_frame(std::size_t size) {
		if (size > MemorySize || used) throw std::bad_alloc();
		used = true;
		return buffer;
	}
};

// -----------------------------------------------------------------------------
// 前方宣言: Promise
// -----------------------------------------------------------------------------
template<typename T, typename Context>
struct SharedTaskPromise;

// -----------------------------------------------------------------------------
// 2. SharedTask (汎用ハンドル)
// -----------------------------------------------------------------------------
template<typename T, typename Context = SharedTaskContext<T>>
struct SharedTask {
	using promise_type = SharedTaskPromise<T, Context>;

	std::weak_ptr<Context> weak_context;

	SharedTask() = default;

	explicit SharedTask(const std::shared_ptr<Context> &context) : weak_context(context) {}

	struct Awaiter {
		std::shared_ptr<Context> context;
		SharedTaskAwaiterNode node;

		bool await_ready() {
			return context->is_ready();
		}

		bool await_suspend(std::coroutine_handle<> caller) {
			node.continuation = caller;
			return context->try_await(&node);
		}

		T await_resume() {
			return context->get_result();
		}
	};

	Awaiter operator co_await() const {
		if (auto context = weak_context.lock()) {
			return Awaiter{std::move(context)};
		} else {
			std::terminate();
		}
	}
};

// -----------------------------------------------------------------------------
// SharedTaskPromiseBase: return_value / return_void の切り分けを担当
// -----------------------------------------------------------------------------

// 1. プライマリテンプレート (T != void)
template<typename T, typename Context>
struct SharedTaskPromiseBase {
	std::weak_ptr<Context> weak_context;

	void return_value(const T& v) {
		if (auto context = weak_context.lock()) {
			context->return_value(v);
		} else {
			std::terminate();
		}
	}

	void return_value(T&& v) {
		if (auto context = weak_context.lock()) {
			context->return_value(std::move(v));
		} else {
			std::terminate();
		}
	}

	void unhandled_exception() {
		if (auto context = weak_context.lock()) {
			context->unhandled_exception();
		} else {
			std::terminate();
		}
	}
};

// 2. void 特化 (T == void)
template<typename Context>
struct SharedTaskPromiseBase<void, Context> {
	std::weak_ptr<Context> weak_context;

	void return_void() {
		if (auto context = weak_context.lock()) {
			context->return_void();
		} else {
			std::terminate();
		}
	}

	void unhandled_exception() {
		if (auto context = weak_context.lock()) {
			context->unhandled_exception();
		} else {
			std::terminate();
		}
	}
};

// -----------------------------------------------------------------------------
// 3. SharedTaskPromise (Baseを継承して実装)
// -----------------------------------------------------------------------------
template<typename T, typename Context>
struct SharedTaskPromise : SharedTaskPromiseBase<T, Context> {

	template<typename... Args>
	SharedTaskPromise(std::allocator_arg_t, const std::shared_ptr<Context>& context, Args&&...) {
		this->weak_context = context;
	}

	SharedTask<T, Context> get_return_object() {
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

		std::coroutine_handle<> await_suspend(std::coroutine_handle<SharedTaskPromise> h) noexcept {
			h.promise().weak_context.lock()->notify_waiters();
			return std::noop_coroutine();
		}
		void await_resume() noexcept {}
	};

	FinalAwaiter final_suspend() noexcept { return {}; }

	template<typename... Args>
	static void* operator new(std::size_t size, std::allocator_arg_t, const std::shared_ptr<Context>& context, Args&&...) {
		return context->allocate_frame(size);
	}

	static void operator delete(void*, std::size_t) {}
};

} // namespace KaitoTokyo::Async
