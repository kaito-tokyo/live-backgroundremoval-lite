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

namespace KaitoTokyo::Async {

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

} // namespace KaitoTokyo::Async
