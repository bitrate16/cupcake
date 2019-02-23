#pragme once

#include <thread>
#include <vector>
#include <condition_variable>

#include "GC"
#include "lock_queue"

namespace ck_core {
	class ckthread {
		friend class GIL;
		
		// Set to 1 if ckthread has natie instance attached
		bool has_native = 0;
		std::thread* native_thread = nullptr;
		
		// Set to 0 if thread is sed and no more operating
		bool is_alive = 1;
		// Set to 1 while thread called GIL::io_lock() for io operations
		bool is_blocked = 0;
		// Set to 1 while thread called GIL::accept_lock() for accepting controller's lock
		bool is_locked = 0;
		
		ckthread() : native_thread_id(std::this_thread::get_id()), thread_id(++threads_total) {};
		
		ckthread(std::thread* t) {
			if (!t)
				throw "ckthread is null";
			
			native_thread = t;
			thread_id = ++threads_total;
			native_thread_id = t->get_id();
			has_native = 1;
		};
		
		~ckthread() {
			// XXX: Solve the problem of destructor
		};
		
	public:
		static ckthread* current_thread = nullptr;
		static int threads_total = 0;
		
		const int thread_id        = -1;
		const int native_thread_id = -1;
		
		inline bool is_alive() {
			return is_alive;
		};
		
		inline bool is_blocked() {
			return is_blocked;
		};
		
		inline bool is_locked() {
			return is_locked;
		};
		
		inline bool locked() {
			return !alive || is_locked || is_blocked;
		};
	};
	
	class GIL {
		// Array of all created threads
		std::vector<ckthread> threads;
		
		// Protector of threads array
		std::mutex vector_threads_lock;
		
		// Main synchronization mutex & conditional
		std::condtional_variable sync_condition;
		std::mutex               sync_mutex;
		
		// Queue for lock requests
		shared_lock_queue sync_lock(sync_mutex, sync_condition);
		
		// Set to 1 if lock is requested by somebody
		bool lock_requested = 0;
		
		GIL();
		~GIL();
		
		// Add current thread into sync_lock list.
		// After pass set lock_requested to 1 and wait for other threads to lock.
		// Returns 1 if locked successfull, 0 else.
		bool request_lock();
		
		// Tries to request lock of other threads.
		// Calls sync_lock::try_lock().
		// Returns 1 if passed and lock is acquired, lock_requested=1, and 0 if it doesn't.
		bool try_request_lock();
		
		// Unlocks all threads. Notifies sync_condition and sync_mutex.
		// Returns 1 if current thread was locking others, 0 else.
		bool free_lock();
		
		// Thread checks if lock_requested flag set to 1.
		// If lock was requested, thread will block till 
		// the operator thread will notify for unblock.
		// Returns 1 if was locked, 0 else.
		bool accept_lock();
		
		// Mark current thread performing IO operatios.
		// Returns 1 if passed, 0 else.
		bool io_lock();
		
		// Unmark current thread performing IO operations and do GIL::accept_lock() if needed.
		// Returns 1 if was performing IO, 0 else and do nothing.
		bool io_unlock();
		
		// Spawns a new thread and registers it in GIL::threads.
		// On start, GIL global instance is assigned to it and then 
		// body function is called.
		void spawn_thread(std::function<void ()> body);
		
		// Locks current thread (current = current_ckthread())
		// with given lambda-condition untill controlling thread 
		// releases lock with notify_all().
		// Not safe for mutex in value access or i/o
		void lock_for_condition(std::function<bool ()> condition_lambda);
		
		// Locks current thread (current = current_ckthread())
		// with given lambda-condition untill controlling thread 
		// releases lock with notify_all() or timeout of WAIT_FOR_PERIOD period.
		// Not safe for mutex in value access or i/o
		void lock_for_time_condition(std::function<bool ()> condition_lambda, long wait_delay = 0);
		
		// Used to notify sync_condition to check it's condition.
		// Used in gil_sync::GIL_NOTIFY_SYNC_LOCK to avoid situation
		// whan all threads come blocked at the same moment as 
		// operator thread in GIL::lock_threads() begin waiting to nothing
		// and all program hangs.
		void notify_sync_lock();
	};
}