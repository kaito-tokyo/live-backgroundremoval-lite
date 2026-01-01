/*
 * KaitoTokyo TaskQueue Library
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>

#include <KaitoTokyo/Logger/ILogger.hpp>

namespace KaitoTokyo::TaskQueue {

/**
 * @brief A self-contained thread queue for executing cancellable tasks with a limit.
 *
 * This class manages a single internal worker thread in an RAII style.
 * The thread is started upon object construction and safely joined upon destruction.
 * If the queue is full when a new task is pushed, the oldest task is cancelled and removed.
 */
class ThrottledTaskQueue {
public:
	/**
     * @brief A cancellation token used to safely share the cancellation state.
     * When set to true, it indicates that the task should be cancelled.
     */
	using CancellationToken = std::shared_ptr<std::atomic<bool>>;

	/**
     * @brief The type of the function that can be added to the queue as a task.
     * @param token A cancellation token unique to this task.
     */
	using CancellableTask = std::function<void(const CancellationToken &)>;

private:
	using QueuedTask = std::pair<std::function<void()>, CancellationToken>;

public:
	/**
     * @brief Constructor. Starts the worker thread.
     * @param logger The logger to use for internal messages.
     * @param maxQueueSize The maximum number of tasks the queue can hold. Must be at least 1.
     */
	ThrottledTaskQueue(std::shared_ptr<const Logger::ILogger> logger, std::size_t maxQueueSize)
		: logger_(std::move(logger)),
		  maxQueueSize_(maxQueueSize > 0 ? maxQueueSize
						 : throw std::invalid_argument("maxQueueSize must be greater than 0")),
		  worker_(&ThrottledTaskQueue::workerLoop, this)
	{
	}

	/**
     * @brief Destructor. Stops the queue and waits for the worker thread to finish.
     */
	~ThrottledTaskQueue() noexcept { shutdown(); }

	// Forbid copy and move semantics to keep ownership simple.
	ThrottledTaskQueue(const ThrottledTaskQueue &) = delete;
	ThrottledTaskQueue &operator=(const ThrottledTaskQueue &) = delete;
	ThrottledTaskQueue(ThrottledTaskQueue &&) = delete;
	ThrottledTaskQueue &operator=(ThrottledTaskQueue &&) = delete;

	/**
	 * @brief Stops the queue and waits for the worker thread to finish.
	 */
	void shutdown() noexcept
	{
		if (worker_.joinable()) {
			stop();
			worker_.join();
		}
	}

	/**
     * @brief Pushes a cancellable task to the queue.
     * @param userTask The task to be executed. It receives a cancellation token as an argument.
     * @return A token that can be used to cancel the task externally.
     * @throws std::runtime_error if the queue has already been stopped.
     */
	CancellationToken push(CancellableTask userTask)
	{
		auto token = std::make_shared<std::atomic<bool>>(false);

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (stopped_) {
				throw std::runtime_error("push on stopped ThrottledTaskQueue");
			}

			// If the queue is full, cancel and remove the oldest task.
			while (queue_.size() >= maxQueueSize_) {
				if (queue_.front().second) {
					queue_.front().second->store(true); // Cancel
				}
				queue_.pop();
			}
			queue_.push({[userTask, token] { userTask(token); }, token});
		}
		cond_.notify_one();
		return token;
	}

private:
	/**
     * @brief The main loop for the worker thread.
     */
	void workerLoop()
	{
		while (true) {
			std::optional<QueuedTask> queuedTaskOpt = pop();
			if (!queuedTaskOpt) {
				break;
			}

			{
				std::lock_guard<std::mutex> lock(mutex_);
				currentTaskToken_ = queuedTaskOpt->second;
			}

			if (currentTaskToken_->load()) {
				{
					std::lock_guard<std::mutex> lock(mutex_);
					currentTaskToken_.reset();
				}
				continue;
			}

			try {
				queuedTaskOpt->first();
			} catch (const std::exception &e) {
				logger_->error("TaskExceptionError", {{"message", e.what()}});
			} catch (...) {
				logger_->error("TaskUnknownExceptionError");
			}

			{
				std::lock_guard<std::mutex> lock(mutex_);
				currentTaskToken_.reset();
			}
		}
	}

	/**
     * @brief Pops a task from the queue. Waits if the queue is empty.
     * @return The task to be executed, or std::nullopt if the queue is stopped and empty.
     */
	std::optional<QueuedTask> pop()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { return !queue_.empty() || stopped_; });

		if (stopped_ && queue_.empty()) {
			return std::nullopt;
		}

		auto queuedTask = std::move(queue_.front());
		queue_.pop();

		return queuedTask;
	}

	/**
     * @brief Stops the queue.
     * Stops accepting new tasks and signals cancellation to all pending tasks in the queue.
     */
	void stop()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (stopped_) {
				return;
			}
			stopped_ = true;

			// Cancel all pending tasks in the queue before shutting down.
			while (!queue_.empty()) {
				if (queue_.front().second) {
					queue_.front().second->store(true);
				}
				queue_.pop();
			}

			if (currentTaskToken_) {
				currentTaskToken_->store(true);
			}
		}
		cond_.notify_all();
	}

	const std::shared_ptr<const Logger::ILogger> logger_;
	const std::size_t maxQueueSize_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::queue<QueuedTask> queue_;
	bool stopped_ = false;
	CancellationToken currentTaskToken_;
	std::thread worker_;
};

} // namespace KaitoTokyo::TaskQueue
