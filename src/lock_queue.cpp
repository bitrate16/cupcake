#include "lock_queue"

void ck_sync::lock_queue::lock() (std::function<void ()> on_blocked = lock_queue_none,   // Called before thread waits lock
							 std::function<void ()> on_unblocked = lock_queue_none, // Called on thread acquires lock
							 std::function<void ()> on_first = lock_queue_none) {   // Called when thread is first
	{
		// Add self into queue
		std::unique_lock<std::mutex> locker(mutex2);
		if (queue.empty()) {
			// Warning: not checking for untracked threads
			queue.push(std::this_thread::get_id());
			on_first();
			return;
		}
	}
	
	on_locked();
	
	std::unique_lock<std::mutex> lk(mutex1);
	cv.wait(lk, [&]{ return queue.front() == std::this_thread::get_id(); });
	
	on_unlocked();
};

bool ck_sync::lock_queue::unlock() {
	// Remove self from queue
	std::unique_lock<std::mutex> locker(mutex2);
	if (queue->front() != std::this_thread::get_id())
		return 0;
	
	queue.pop();
	cv.notify_all();
	return 1;
};