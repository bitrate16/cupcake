#pragma once

#include <thread>
#include <vector>
#include <condition_variable>

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
	const std::function<void ()> GIL_LOCK_NOTIFY_SYNC_LOCK = [&]{ 
		// Locks this mutex by current thread.
		// That means that when GIL::lock_threads is called,
		// operator thread will hold the mutex till it will enter waiting. 
		// (see doc for std::condition_variable)
		// This thread will attempt to acquire the mutex, but
		// operator is owning it now and will block untill call wait.
		std::unique_lock<std::mutex> lk(gil->sync_lock);
		ck_thread *t = gil->current_ckthread_noexcept();
		if (t) ++t->is_blocked;
		gil->notify_sync_lock();
	};
	
	// Default parameter must be passed when some thread is joining GIL sync_lock
	// Notifies current thread that it is blocked.
	// Also calls GIL::notify_sync_lock() to force waiting operator 
	// to wake and process when all threads are blocked to next operations.
	// Doesn't not lock global mutex. Unsafe while operator thread might loose notifications.
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
		// Default constructor that assigns std::this_thared::get_id() to std_thread_id.
		ck_thread() : std_thread_id(std::this_thread::get_id()) {};
		
		// Constructor that takes pointer to created thread and assigns it to 
		// thread and it's id to std_thread_id.
		// Warning: no nullptr check
		ck_thread(std::thread *t) : std_thread_id(t->get_id()), thread(t) {};
		
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
		// Set to 1 if thread is still alive.
		// Changed to 0 when body function of the thread is finished working 
		// and thread requests GC to collect it.
		int is_alive = 1;
		// Id of std::this_thread::get_id() value for current thread.
		// Used for quicksearch and define threads that is not created by constructor.
		int std_thread_id = -1;
		
		// Unique thread number in ck debug
		int thread_id = -1;
		// Set to 1 if thread accepted lock request and waiting for signal
		int accepted_lock = 0;
		// Interrupt code handling
		int interrupt = -1;
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
		// sync state thread for locking current input or mutex lock make wait condition variable at postion local handler thread.
	};
	
	/*
	 * Global Interpreter Lock type
	 * Single for each thread, provides global synchronization, thread control,
	 * garbage collector access.
	 */
	class GIL {
		public:
		// Used for making all threads pause on lock_threads() made by controller thread.
		// Provides synchronization for multiple threads trying to pause on operator request.
		std::condtional_variable sync_condition;
		std::mutex sync_mutex;
		// Locked while one thread tries to request global lock of all ohther threads.
		// Using shared lock to make all threaads 
		shared_lock_queue sync_lock(sync_mutex, sync_condition);
		// Lock signal flag. 
		// Operated by GIL::lock_threads. Set to 1 when threads
		// are softly requested to block to allows operator 
		// perform something in hardly synchronized single-threaded mode.
		// Usually when using block, gil_sync::GIL_LOCK_NOTIFY_SYNC_LOCK has to be called.
		int lock_requested = 0;
		// List of all spawned and registered threads.
		// Each thread that accesses methods of GIL must be
		// thacked by it and initialized by GIL::spawn_thread().
		// Any untracked thread access to GIL WILL CAUSE ERRORS. 
		std::vector<ck_thread*> threads;
		// Threads vector protector
		std::mutex threads_lock;
		// Garbage collector
		GC gc;
		// Signal value. Only one thread at time can access it.
		std::atomic<int> signal = -1;
		
		// GIL instance is created in main thread.
		// On create it adds main thread as first in GIL::threads.
		// Main thread do not destroy GIL instance on exit.
		// GIL instance is destroyed by latest survived thread.
		// In fact main thread do not need the destruction.
		// It is destructed on program exit. in cases:
		// 1. Main finished and there is no child threads.
		// .... Main calls GIL::destruct();
		// .... GIL::destruct() calls GC::dispose(), disposes all ck_threads
		// .... And finally program safety terminates.
		// 2. Main finished and there is child threads.
		// .... The last thread safety terminates it's work.
		// .... After main finished, it's been put on waiting for 
		// .... other threads to die themselves. 
		// .... Once last thread called GIL::notify_sync_lock()
		// .... main thread will unblock and clear all data and finish execution.
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
		
		// Spawns a new thread and registers it in GIL::threads.
		// On start, gil global instance is assigned to it and then 
		// body function is called.
		void spawn_thread(std::function<void ()> body);
		
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
		void lock_for_condition(std::function<bool ()> condition_lambda);
		
		// Locks current thread (current = current_ckthread())
		// with given lambda-condition untill controlling thread 
		// releases lock with notify_all() or timeout of WAIT_FOR_PERIOD period.
		// Not safe for mutex in value access or i/o
		void lock_for_time_condition(std::function<bool ()> condition_lambda, long wait_delay = -1);
		
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