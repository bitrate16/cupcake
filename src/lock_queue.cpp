#include "lock_queue.h"

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>


void ck_sync::lock_queue::lock (std::function<void ()> on_blocked,   // Called before thread waits lock
							    std::function<void ()> on_unblocked, // Called on thread acquires lock
							    std::function<void ()> on_first) {   // Called when thread is first
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
	
	on_blocked();
	
	std::unique_lock<std::mutex> lk(mutex1);
	cv.wait(lk, [&]{ return queue.front() == std::this_thread::get_id(); });
	
	on_unblocked();
};

bool ck_sync::lock_queue::try_lock(std::function<void ()> on_first) {   // Called when thread is first
	// Add self into queue
	std::unique_lock<std::mutex> locker(mutex2);
	if (queue.empty()) {
		// Warning: not checking for untracked threads
		queue.push(std::this_thread::get_id());
		on_first();
		return 1;
	} else return 0;
};

bool ck_sync::lock_queue::unlock() {
	// Acquire condition_variable lock
	std::lock_guard<std::mutex> guard(mutex1);
	// Remove self from queue
	std::unique_lock<std::mutex> locker(mutex2);
	if (queue.front() != std::this_thread::get_id())
		return 0;
	
	queue.pop();
	cv.notify_all();
	return 1;
};


void ck_sync::shared_lock_queue::lock (std::function<void ()> on_blocked,   // Called before thread waits lock
							           std::function<void ()> on_unblocked, // Called on thread acquires lock
							           std::function<void ()> on_first) {   // Called when thread is first
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
	
	on_blocked();
	
	std::unique_lock<std::mutex> lk(mutex1);
	cv.wait(lk, [&]{ return queue.front() == std::this_thread::get_id(); });
	
	on_unblocked();
};

void ck_sync::shared_lock_queue::shared_lock (std::unique_lock<std::mutex> &lock,  // Passing lock by reference allows 
											  									   // acquiring it on the outside.
											  									   // Lock expected to be locked :D
											  std::function<void ()> on_blocked,   // Called before thread waits lock
											  std::function<void ()> on_unblocked, // Called on thread acquires lock
											  std::function<void ()> on_first) {   // Called when thread is first
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
	
	on_blocked();
	
	// std::unique_lock<std::mutex> lk(mutex1); <---|
	// |--------------------------------------------|
	// |
	// Equals to:
	// 
	// std::mutex myMutex;
	// std::condition_variable cv;
	// shared_lock_queue qlock(myMutex, cv);
	// std::unique_lock<std::mutex> mlock(myMytex);
	// qlock.shared_lock(lock=mlock);
	// 
	// Ater unblock shared lock will be acquired by current thread.
	cv.wait(lock, [&]{ return queue.front() == std::this_thread::get_id(); });
	
	on_unblocked();
};

bool ck_sync::shared_lock_queue::try_lock(std::function<void ()> on_first) {   // Called when thread is first
	// Add self into queue
	std::unique_lock<std::mutex> locker(mutex2);
	if (queue.empty()) {
		// Warning: not checking for untracked threads
		queue.push(std::this_thread::get_id());
		on_first();
		return 1;
	} else return 0;
};

bool ck_sync::shared_lock_queue::unlock() {
	// Acquire condition_variable lock
	std::unique_lock<std::mutex> guard(mutex1);
	// Remove self from queue
	std::lock_guard<std::mutex> locker(mutex2);
	if (queue.front() != std::this_thread::get_id())
		return 0;
	
	queue.pop();
	cv.notify_all();
	return 1;
};