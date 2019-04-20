#pragma once

#include <thread>
#include <vector>
#include <condition_variable>
#include <atomic>

#include "exceptions.h"
#include "pthread_util.h"
#include "lock_queue.h"


namespace ck_core {
	
	class ckthread {
		friend class GIL;
		
		// Set to 1 if current instance has native attached
		bool has_native = 0;
		// Contains instance of the native thread
		ck_pthread::thread* native_thread = nullptr;
		// Contains native thread id for faster use.
		uint64_t native_thread_id = 0;
		
		// Thread state is described by values below.
		//  is alive is equals to 1 then thread is still running in normal mode.
		//  is dead is equals to 1 when thread is dieing and performing after-kill handler execution.
		//  is running is equal to 1 when thread is running and is sef to 0, thread instantly stops.
		//  is locked set to 1 is thread allowed operator to acquire lock on it.
		//  is blocked is set to 1 if thread may perform IO operations.
		//  Performing kill twice on a single thread makes it stop.
		
		bool alive   = 1;
		bool dead    = 0;
		bool running = 1;
		bool blocked = 0;
		bool locked  = 0;
		
		// Passed only from GIL
		ckthread(ck_pthread::thread* t) {
			if (!t) throw ck_exceptions::InvalidState(L"Thread is null");
			
			native_thread    = t;
			// thread_id        = (long long) t->get_id();
			native_thread_id = t->get_id();
			has_native       = 1;
		};
		
		ckthread() : native_thread_id(ck_pthread::thread::this_thread().get_id()) {};
		
	public:
		
		~ckthread() {
			// Logically here goes nothing because when instance of this object 
			// is being destroyed, thread is finishing it's work.
			
			// XXX: Solve the problem of destructor
		};
		
		inline bool is_alive() {
			return alive;
		};
		
		inline bool is_dead() {
			return dead;
		};
		
		inline bool is_running() {
			return running;
		};
		
		inline bool is_blocked() {
			return blocked;
		};
		
		inline bool is_locked() {
			return locked;
		};
		
		// kill num:  0        1        2
		//    alive | 1 | -> | 0 | -> | 0 |
		//     dead | 0 | -> | 1 | -> | 1 |
		//  running | 1 | -> | 1 | -> | 0 |
		// 
		inline bool kill() {
			if (running)
				if (alive) {
					dead = 1;
					alive = 0;
				} else if (dead) {
					dead = 1;
					alive = 0;
					running = 0;
				}
		};
		
		// Set running state
		inline bool set_running(bool is_running) {
			running = is_running;
		};
		
		// Set blocked by i/o 
		inline bool set_blocked(bool is_blocked) {
			blocked = is_blocked;
		};
		
		// Set locked by other threads 
		inline bool set_locked(bool is_locked) {
			locked = is_locked;
		};
		
		// Returns whenever thared is allowing other threads to acquire GIL lock
		inline bool locked_state() {
			return !running || locked || blocked;
		};
		
		inline uint64_t get_native_id() {
			return native_thread_id;
		};
	
		// Set blocked = locked = 0
		inline void clear_blocks() {
			blocked = locked = 0;
		};
		
		// Resets state of the thread to the default
		inline void restate() {
			alive = 1;
			dead = 0;
			running = 1;
		};
	};
	
	// Forward Pointers
	class GC;
	class ck_executer;
	
	class GIL {
	
		// friend int main(int argc, const char** argv);
		
		// Array of all created threads
		std::vector<ckthread*> threads;
		// Protector of threads array
		ck_pthread::mutex vector_threads_lock;
		
		// Main synchronization mutex & conditional var
		ck_pthread::cond_var sync_condition;
		ck_pthread::mutex    sync_mutex;
		
		// Queue for lock requests
		// ck_sync::shared_lock_queue sync_lock;
		
		// Set to 1 if lock is requested by someone
		bool lock_requested = 0;
	
		// Points to the current ckthread
		// Assigned when thread is being spawned via 
		// spawn_thread or creation of GIL
		static thread_local ckthread* current_thread_ptr; // zero-initialized
		
		// Assigned on thread creation. Thread-local instance of executer.
		static thread_local ck_executer* executer;
	
		// Pointer to itself
		// Assigned when thread is being spawned via 
		// spawn_thread or creation of GIL
		static GIL* gil_instance; // zero-initialized
		
		// Instance of Garbage Collector
		GC* gc; // zero-initialized
	
	public:
		
		GIL();
		~GIL();
		
		inline static ckthread* current_thread() {
			return GIL::current_thread_ptr;
		};
		
		inline static GIL* instance() {
			return GIL::gil_instance;
		};
		
		inline static GC* gc_instance() {
			return GIL::instance()->gc;
		};
		
		inline static ck_executer* executer_instance() {
			return GIL::instance()->executer;
		};
		
		// Marks all threads with is_alive(0) for forcing them to finish their work.
		// Used when no __defsignalhandler is not set for tha main thread.
		void terminate();
		
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
		// After body finishes, thread is being removed from threads list.
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



//              (        (
//              ( )      ( )          (
//       (       Y        Y          ( )
//      ( )     |"|      |"|          Y
//       Y      | |      | |         |"|
//      |"|     | |.-----| |---.___  | |
//      | |  .--| |,~~~~~| |~~~,,,,'-| |
//      | |-,,~~'-'      '-'       ~~| |._
//     .| |~                         '-',,'.
//    /,'-'   ##   #### ####              ~,\
//   / ;      ##    ##  ##>    _____________;_)
//   | ;      #### #### ####  `'-._          |
//   | ;                           '-._      |
//   |\ ~,,,                      ,,,,,'-.   |
//   | '-._ ~~,,,            ,,,~~ __.-'~ |  |
//   |     '-.__ ~~~~~~~~~~~~ __.-'       |__|
//   |\         `'----------'`           _|
//   | '=._                         __.=' |
//   :     '=.__               __.='      |
//    \         `'==========='`          .'
// lie '-._                         __.-'
//         '-.__               __.-'
//              `'-----------'`