// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

#include "Task.hpp"

namespace KaitoTokyo::Async {

struct [[nodiscard]] JoinTask {
	struct promise_type {
		JoinTask get_return_object()
		{
			return JoinTask{std::coroutine_handle<promise_type>::from_promise(*this)};
		}
		std::suspend_always initial_suspend() noexcept { return {}; }

		std::suspend_always final_suspend() noexcept { return {}; }

		void return_void() noexcept {}
		[[noreturn]] void unhandled_exception() { std::terminate(); }
	};

	explicit JoinTask(std::coroutine_handle<promise_type> h) : handle(h) {}

	~JoinTask()
	{
		if (handle)
			handle.destroy();
	}

	// Move only
	JoinTask(const JoinTask &) = delete;
	JoinTask &operator=(const JoinTask &) = delete;
	JoinTask(JoinTask &&other) noexcept : handle(other.handle) { other.handle = nullptr; }
	JoinTask &operator=(JoinTask &&other) noexcept
	{
		if (this != &other) {
			if (handle) {
				handle.destroy();
			}
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}

	void start()
	{
		if (handle && !handle.done())
			handle.resume();
	}

	bool is_done() const { return handle && handle.done(); }

private:
	std::coroutine_handle<promise_type> handle;
};

struct JoinState {
	std::mutex mutex;
	std::condition_variable cv;
	bool done = false;
	std::exception_ptr error = nullptr;
};

inline void join(Task<void> task)
{
	if (!task)
		return;

	JoinState state;

	auto wrapper = [&state](Task<void> t) -> JoinTask {
		try {
			co_await t;
		} catch (...) {
			state.error = std::current_exception();
		}

		{
			std::scoped_lock lock(state.mutex);
			state.done = true;
		}
		state.cv.notify_one();
	};

	auto join_task = wrapper(std::move(task));
	join_task.start();

	{
		std::unique_lock lock(state.mutex);
		state.cv.wait(lock, [&state] { return state.done; });
	}

	while (!join_task.is_done()) {
		std::this_thread::yield();
	}

	if (state.error) {
		std::rethrow_exception(state.error);
	}
}

} // namespace KaitoTokyo::Async
