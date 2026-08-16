// core/thread_safe_queue.h -- a small bounded producer/consumer queue (C++,
// header-only).
//
// Built for the image loaders: the main (producer) thread pushes load requests,
// a worker (consumer) thread pops them, blocking when there's nothing to do.
// When a capacity is set and the queue is full, push() drops the OLDEST item so
// the newest wins -- exactly right for backgrounds/thumbnails, where a request
// for a folder the user has already navigated away from is worthless.
//
// This replaces the hand-rolled SDL_mutex/SDL_cond linked-list queues whose
// late size-counter decrement and cross-thread task ownership caused the
// memory corruption that the POC had to disable. Correctness lives in one small
// place now instead of being re-implemented per queue.

#ifndef NEXTUI_CORE_THREAD_SAFE_QUEUE_H
#define NEXTUI_CORE_THREAD_SAFE_QUEUE_H

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

namespace core {

template <class T>
class ThreadSafeQueue {
public:
	// capacity == 0 means unbounded.
	explicit ThreadSafeQueue(size_t capacity = 0) : capacity_(capacity) {}

	void push(T item) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (capacity_ && queue_.size() >= capacity_) queue_.pop(); // drop oldest
		queue_.push(std::move(item));
		cond_.notify_one();
	}

	// Blocks until an item is available. Returns false once shutdown() has been
	// called and the queue has drained -- the signal for a consumer to exit.
	bool pop(T& out) {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
		if (queue_.empty()) return false;
		out = std::move(queue_.front());
		queue_.pop();
		return true;
	}

	void shutdown() {
		std::lock_guard<std::mutex> lock(mutex_);
		shutdown_ = true;
		cond_.notify_all();
	}

private:
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
	size_t capacity_;
	bool shutdown_ = false;
};

} // namespace core

#endif // NEXTUI_CORE_THREAD_SAFE_QUEUE_H
