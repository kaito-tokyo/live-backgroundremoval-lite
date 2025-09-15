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

#include "obs-bridge-utils/ILogger.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

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

	const kaito_tokyo::obs_bridge_utils::ILogger &logger;
	const std::size_t maxQueueSize;
	std::thread worker;
	std::mutex mtx;
	std::condition_variable cond;
	std::queue<QueuedTask> queue;
	bool stopped = false;

	std::mutex taskMutex;
	std::condition_variable taskCond;
	int activeTaskCount = 0;

public:
	/**
     * @brief Constructor. Starts the worker thread.
     * @param _logger The logger to use for internal messages.
     * @param max_size The maximum number of tasks the queue can hold. Must be at least 1.
     */
	ThrottledTaskQueue(const kaito_tokyo::obs_bridge_utils::ILogger &_logger, std::size_t _maxQueueSize)
		: logger(_logger),
		  maxQueueSize(_maxQueueSize),
		  worker(&ThrottledTaskQueue::workerLoop, this)
	{
		assert(_maxQueueSize > 0 && "max_size must be greater than 0");
	}

	/**
     * @brief Destructor. Stops the queue and waits for the worker thread to finish.
     */
	~ThrottledTaskQueue()
	{
		if (worker.joinable()) {
			stop();
			worker.join();
		}
	}

	// Forbid copy and move semantics to keep ownership simple.
	ThrottledTaskQueue(const ThrottledTaskQueue &) = delete;
	ThrottledTaskQueue &operator=(const ThrottledTaskQueue &) = delete;
	ThrottledTaskQueue(ThrottledTaskQueue &&) = delete;
	ThrottledTaskQueue &operator=(ThrottledTaskQueue &&) = delete;

	/**
     * @brief Pushes a cancellable task to the queue.
     * @param user_task The task to be executed. It receives a cancellation token as an argument.
     * @return A token that can be used to cancel the task externally.
     * @throws std::runtime_error if the queue has already been stopped.
     */
	CancellationToken push(CancellableTask user_task)
	{
		auto token = std::make_shared<std::atomic<bool>>(false);

		{
			std::lock_guard<std::mutex> lock(mtx);
			if (stopped) {
				throw std::runtime_error("push on stopped ThrottledTaskQueue");
			}

			// If the queue is full, cancel and remove the oldest task.
			while (queue.size() >= maxQueueSize) {
				if (queue.front().second) {
					queue.front().second->store(true); // Cancel
				}
				queue.pop();
			}
			queue.push({[user_task, token] { user_task(token); }, token});
		}
		cond.notify_one();
		return token;
	}

	/**
	 * @brief Cancels all pending tasks and waits for the currently executing task to complete.
	 */
	void cancelAllAndWait()
	{
		{
			std::lock_guard<std::mutex> lock(mtx);
			// Cancel and remove all tasks in the queue.
			while (!queue.empty()) {
				if (queue.front().second) {
					queue.front().second->store(true);
				}
				queue.pop();
			}
		}

		// Wait for the currently running task to finish.
		{
			std::unique_lock<std::mutex> lock(taskMutex);
			taskCond.wait(lock, [this] { return activeTaskCount == 0; });
		}
	}

private:
	/**
     * @brief The main loop for the worker thread.
     */
	void workerLoop()
	{
		while (true) {
			std::optional<std::function<void()>> taskOpt = pop();
			if (!taskOpt) {
				break;
			}

			{
				std::lock_guard<std::mutex> lock(taskMutex);
				activeTaskCount++;
			}

			try {
				(*taskOpt)();
			} catch (const std::exception &e) {
				logger.error("ThrottledTaskQueue: Task threw an exception: {}", e.what());
			} catch (...) {
				logger.error("ThrottledTaskQueue: Task threw an unknown exception.");
			}

			{
				std::lock_guard<std::mutex> lock(taskMutex);
				activeTaskCount--;
			}
			taskCond.notify_all();
		}
	}

	/**
     * @brief Pops a task from the queue. Waits if the queue is empty.
     * @return The task to be executed, or std::nullopt if the queue is stopped and empty.
     */
	std::optional<std::function<void()>> pop()
	{
		std::unique_lock<std::mutex> lock(mtx);
		cond.wait(lock, [this] { return !queue.empty() || stopped; });

		if (stopped && queue.empty()) {
			return std::nullopt;
		}

		auto task_pair = std::move(queue.front());
		queue.pop();

		return std::move(task_pair.first);
	}

	/**
     * @brief Stops the queue.
     * Stops accepting new tasks and signals cancellation to all pending tasks in the queue.
     */
	void stop()
	{
		{
			std::lock_guard<std::mutex> lock(mtx);
			if (stopped) {
				return;
			}
			stopped = true;

			while (!queue.empty()) {
				if (queue.front().second) {
					queue.front().second->store(true);
				}
				queue.pop();
			}
		}
		cond.notify_all();
	}
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
