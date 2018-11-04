#pragma once

#include <queue>
#include <thread>

namespace ck_sync {
	// Empty lambda for lock_queue::lock()
	const std::function<void ()> lock_queue_none = []{};
	
	/*
	 * Lock queue if used for queiung lock requests in a single nutex.
	 * When thread is calling lock_queue::lock() it's been put
	 * in waiting for front threads to sequently finish their work.
	 * When thread is calling lock_queue::unlock() it is removing itself 
	 * from queue and all other threads are notified about that.
	 * Next in sequence thread is being unblocked and acquires lock 
	 * on the mutex.
	 */
	class lock_queue {
		// Sync mutex
		// XXX: Maybe use recursive?
		std::mutex mutex1; 
		// Protect mutex
		std::mutex mutex2;
		// Coditional variable for wait
		std::conditional_variable cv;
		// Threads queue
		std::queue<int> queue;
		
		// Make this thread wait in lock queue
		// on_blocked is called when thread begin vaiting for onlock
		// on_unblocked is called when thread acquired lock
		// on_first is called if thread didn't wait
		void lock(std::function<void ()> on_blocked = lock_queue_none,   // Called before thread waits lock
				  std::function<void ()> on_unblocked = lock_queue_none, // Called on thread acquires lock
				  std::function<void ()> on_first = lock_queue_none);    // Called when thread is first
				  
		// Tries to lock this queue.
		// Completes only if queue is empty.
		// Returns 1 if lock has passed, 0 else.
		// on_first() will be executed if operation successed.
		bool try_lock(std::function<void ()> on_first = lock_queue_none);    // Called when thread is first
		
		// Current thread releases lock, others are notified.
		// Returns 1 if unlock successed.
		bool unlock();
	};
	
	/*
	 * Shared lock queue is equalient to lock queue but mutex and 
	 * conditional_variable are passed to it by reference allowing 
	 * user to use single muutex and cond var to synchronize the queue.
	 * 
	 * Lock queue if used for queiung lock requests in a single nutex.
	 * When thread is calling lock_queue::lock() it's been put
	 * in waiting for front threads to sequently finish their work.
	 * When thread is calling lock_queue::unlock() it is removing itself 
	 * from queue and all other threads are notified about that.
	 * Next in sequence thread is being unblocked and acquires lock 
	 * on the mutex.
	 */
	class shared_lock_queue {
		// Sync mutex
		std::mutex &mutex1; 
		// Protect mutex
		std::mutex mutex2;
		// Coditional variable for wait
		std::conditional_variable &cv;
		// Threads queue
		std::queue<int> queue;
		
		shared_lock_queue(std::mutex &_mtx, conditional_variable &_cv) : mutex1(_mtx), cv(_cv) {};
		
		// Make this thread wait in lock queue
		// on_blocked is called when thread begin vaiting for onlock
		// on_unblocked is called when thread acquired lock
		// on_first is called if thread didn't wait
		void lock(std::function<void ()> on_blocked = lock_queue_none,   // Called before thread waits lock
				  std::function<void ()> on_unblocked = lock_queue_none, // Called on thread acquires lock
				  std::function<void ()> on_first = lock_queue_none);    // Called when thread is first
				  
		// Allows passing mutex1 lock by reference and 
		// operating with it out of the scope.
		void sharedlock(std::unique_lock<std::mutex> &lock,              // Passing lock by reference allows 
																	     // acquiring it on the outside.
																	     // Lock expected to be locked :D
				  std::function<void ()> on_blocked = lock_queue_none,   // Called before thread waits lock
				  std::function<void ()> on_unblocked = lock_queue_none, // Called on thread acquires lock
				  std::function<void ()> on_first = lock_queue_none);    // Called when thread is first
				  
		// Tries to lock this queue.
		// Completes only if queue is empty.
		// Returns 1 if lock has passed, 0 else.
		// on_first() will be executed if operation successed.
		bool try_lock(std::function<void ()> on_first = lock_queue_none);    // Called when thread is first
		
		// Current thread releases lock, others are notified.
		// Returns 1 if unlock successed.
		bool unlock();
	};
};