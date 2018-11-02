#pragma once

#include <thread>
#include <vector>

#include "GC"
#include "lock_queue"

namespace gil_sync {
	// Default parameter must be passed when some thread is joining GIL sync_lock
	// Notifies current thread that it is blocked.
	const std::function<void ()> GIL_SYNC_LOCK = [&]{ 
		ck_thread *t = gil->current_ckthread_noexcept();
		if (t) ++t->is_blocked;
	};
	
	// Default parameter must be passed when some thread is joining GIL sync_lock
	// Notifies current thread that it is blocked.
	// Also calls GIL::notify_sync_lock() to force waiting operator 
	// to wake and process when all threads are blocked to next operations.
	const std::function<void ()> GIL_NOTIFY_SYNC_LOCK = [&]{ 
		ck_thread *t = gil->current_ckthread_noexcept();
		if (t) ++t->is_blocked;
		gil->notify_sync_lock();
	};
	
	// Default parameter must be passed when some thread is joining GIL sync_lock
	// Notifies current thread that it is unblocked.
	const std::function<void ()> GIL_SYNC_UNLOCK = [&]{ 
		ck_thread *t = gil->current_ckthread_noexcept();
		if (t) --t->is_blocked;
	};
};

namespace ck_core {	
	// Delay time of std::condition_variable::wait_for
	const WAIT_FOR_DEFAULT_PERIOD = 100;

	/*
	 * Contains all information about thread used by interpreter.
	 * Information if user for thread synchronization and interrupts.
	 */
	class ck_thread {
		public:
		// Pointer to the assigned thread
		std::thread *thread = nullptr;
		// Set to 1 if thread currently is blocked and lock_request is accepted.
		// If thread is has accepted blocking, that guarantee
		// that GIL::lock_threads() will be safe and will not be
		// interrupted. 
		// This flag can be set by GIL::sync_lock or any other lock_queue
		// that's blocking also depends on GIL::lock_threads() state.
		// This flag can be set by thread while it is waiting for system call result.
		// In this case system call myst be wrapped into std::async and notify thread 
		// about result at the end of system call. Short-period calls like malloc/calloc/free
		// can not be wrapped in std::async. 
		// This flag can be set by thread if it has accepted global block and now waiting
		// for controller thread to call GIL::sync_condition.notify_all().
		int is_blocked = 0;
		// Unique thread number in ck debug
		int thread_id       = -1;
		// Set to 1 if thread accepted lock request and waiting for signal
		int accepted_lock   = 0;
		// Interrupt code handling
		int interrupt       = -1;
		// Set to 1 while thread is performing blocking operation
		// (aka read/write, listed to input or sockets)
		// is_blocked used for safety pausing threads while they are freezed.
		// (aka cin >> variable => is_blocked = 1; GIL.lock_threads can pause this thread)
		// Must be atomic to prevent change after value redden
		std::atomic<bool> is_blocked = 0;
		// Set to 1 if thread is locked on some object.
		// Must be atomic to prevent change after value redden
		std::atomic<bool> is_object_lock = 0;
		// Condition variable used for something thanslate this text
		std::condition_variable cv;
		// sync state thread for locking current input or mutex lock make wait conditional variable at postion local handler thread.
	};
	
	/*
	 * Global Interpreter Lock type
	 * Single for each thread, provides global synchronization, thread control,
	 * garbage collector access.
	 */
	class GIL {
		public:
		// Locked while one thread tries to request global lock of all ohther threads.
		lock_queue sync_lock;
		// Used for making all threads pause on lock_threads() made by controller thread
		std::condtional_variable sync_condition;
		std::mutex sync_mutex;
		// List of all spawned threads
		std::vector<ck_thread*> threads;
		// Garbage collector
		GC gc;
		// Signal value. Only one thread at time can access it.
		std::atomic<int> signal = -1;
		// Lock signal flag. Equals to 1 if threads expected to pause.
		int lock_requested = 0;
		
		GIL();
		~GIL();
		
		// Locks all threads but current.
		// Waits till all threads will receive lock signal and pause.
		// Returns 1 if lock has been successfull, 0 else.
		bool lock_threads(int thread_id = -1);
		
		// Tries to perform GIL lock.
		// Uses to call GIL::sync_lock::try_lock().
		// If result was 1, lock passes and current thread is made operator.
		// Else returns 0 and continues execution.
		// This is only (for now?) way to fully lock GIL and avoid situation 
		// descrbed in GIL::lock_threads().
		bool try_lock_threads();
		
		// Unlocks all threads.
		// Calling unpause on all threads but current.
		// Returns 1 if current thread was blocking and could unblock.
		bool unlock_threads(int thread_id = -1);
	
		// Called to notify GIL about thread performing blocking operation
		bool notify_block(int thread_id = -1);
		bool notify_unblock(int thread_id = -1);
		
		// Called when thread is locking on some object to prevent it's 
		// pause/interrupt on lock_threads().
		bool notify_object_lock(int thread_id = -1);
		bool notify_object_unlock(int thread_id = -1);
		
		// Locks current thread (current = current_ckthread())
		// with given lambda-condition untill controlling thread 
		// releases lock with notify_all().
		// Not safe for mutex in value access or i/o
		void lock_for_notify_condition(std::function<bool ()>);
		
		// Locks current thread (current = current_ckthread())
		// with given lambda-condition untill controlling thread 
		// releases lock with notify_all() or timeout of WAIT_FOR_PERIOD period.
		// Not safe for mutex in value access or i/o
		void lock_for_time_condition(std::function<bool ()>, );
		
		// Returns current ck_thread.
		// Throws error if function was called from non-registered thread.
		ck_thread *current_ckthread();
		
		// Equals to GIL::current_ckthread() but returns nullptr 
		// instead of exception
		ck_thread *current_ckthread_noexcept() noexcept;
		
		// Used to notify sync_condition to check it's condition.
		// Used in gil_sync::GIL_NOTIFY_SYNC_LOCK to avoid situation
		// whan all threads come blocked at the same moment as 
		// operator thread in GIL::lock_threads() begin waiting to nothing
		// and all program hangs.
		void notify_sync_lock();
	};
	
	// Instance of GIL for each thread.
	static GIL *gil;
};