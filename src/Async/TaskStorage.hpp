/*
 * KaitoTokyo Async Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <new>

namespace KaitoTokyo::Async {

struct TaskStorageReleaser {
	bool *usedFlagPtr;

	void operator()(void *) const
	{
		if (usedFlagPtr) {
			*usedFlagPtr = false;
		}
	}
};

using TaskStoragePtr = std::unique_ptr<void, TaskStorageReleaser>;

template<std::size_t Size = 4096> class TaskStorage {
	alignas(std::max_align_t) char buffer_[Size];
	bool used_ = false;

public:
	TaskStorage() = default;
	~TaskStorage() = default;

	TaskStorage(const TaskStorage &) = delete;
	TaskStorage &operator=(const TaskStorage &) = delete;

	TaskStoragePtr allocate(std::size_t n)
	{
		if (n > Size || used_) {
			return TaskStoragePtr(nullptr, TaskStorageReleaser{nullptr});
		}

		used_ = true;
		return TaskStoragePtr(buffer_, TaskStorageReleaser{&used_});
	}
};

} // namespace KaitoTokyo::Async
