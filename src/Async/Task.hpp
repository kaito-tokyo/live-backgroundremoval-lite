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
#include <exception>
#include <new>
#include <utility>
#include <variant>

#include "SymmetricTransfer.hpp"
#include "TaskStorage.hpp"

namespace KaitoTokyo::Async {

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

template<typename T> struct [[nodiscard]] Task {
	struct promise_type : TaskPromiseBase<T> {
		std::coroutine_handle<> continuation_ = nullptr;

		Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }

		std::suspend_always initial_suspend() { return {}; }
		auto final_suspend() noexcept { return SymmetricTransfer<promise_type>{}; }

		template<typename Alloc, typename... Args>
		static void *operator new(std::size_t size, std::allocator_arg_t, Alloc &alloc, Args &&...)
		{
#ifdef NDEBUG
			std::size_t total_size = size + sizeof(TaskStoragePtr);

			auto ptr = alloc.allocate(total_size);

			if (!ptr) {
				throw std::bad_alloc();
			}

			void *raw_ptr = ptr.get();

			new (raw_ptr) TaskStoragePtr(std::move(ptr));

			return static_cast<char *>(raw_ptr) + sizeof(TaskStoragePtr);
#else
			(void)alloc;
			return ::operator new(size);
#endif
		}

		static void *operator new(std::size_t) = delete;

		static void operator delete(void *ptr, std::size_t)
		{
#ifdef NDEBUG
			char *raw_ptr = static_cast<char *>(ptr) - sizeof(TaskStoragePtr);

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
