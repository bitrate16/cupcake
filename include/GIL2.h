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
		// Thread amount counter
		static uint64_t thread_counter;
		uint64_t thread_id;
		
		// Thread state is described by values below.
		//  // is alive is equals to 1 then thread is still running in normal mode.
		//  // is dead is equals to 1 when thread is dieing and performing after-kill handler execution.
		//  is running is equal to 1 when thread is running and is sef to 0, thread instantly stops.
		//  is locked set to 1 is thread allowed operator to acquire lock on it.
		//  is blocked is set to 1 if thread may perform IO operations.
		//  // Performing kill twice on a single thread makes it stop.
		
		bool running = 1;
		bool blocked = 0;
		bool locked  = 0;
		
		// Indicates if this thread is owning GIL lock now.
		// Used to avoid situuations of incorrect GIL locking/unlocking.
		bool own_gil = 0;
		
		// Passed only from GIL
		ckthread(ck_pthread::thread* t) {
			if (!t) throw ck_exceptions::InvalidState(L"Thread is null");
			thread_id = thread_counter++;
			
			native_thread    = t;
			// thread_id        = (long long) t->get_id();
			native_thread_id = t->get_id();
			has_native       = 1;
		};
		
		ckthread() : native_thread_id(ck_pthread::thread::this_thread().get_id()) { thread_id = thread_counter++; };
		
	public:
		
		~ckthread() {
			// Logically here goes nothing because when instance of this object 
			// is being destroyed, thread is finishing it's work.
			
			// XXX: Solve the problem of destructor
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
		
		inline uint64_t count() {
			return thread_counter;
		};
		
		inline uint64_t count_id() {
			return thread_id;
		};
		
		// Resets state of the thread to the default
		inline void restate() {
			running = 1;
		};
	};
	
	// Forward Pointers
	class GC;
	class ck_executer;
	
	class GIL {
		
		// Array of all created threads
		std::vector<ckthread*> threads;
		
		// Main synchronization mutex & conditional var
		ck_pthread::cond_var sync_condition;
		
		// I.
		//  Locked when one of the threads needs to require a special prior access over 
		//   the other threads
		// II.
		//  Locked when new thread is being created or old thread is being destroyed.
		//  Also locked during main checking other threads for their running state.
		//   When main thread finishes it's work, it has to wait on sync_condition
		//   till other threads will finish their work. To avoid priority race main
		//   should lock the mutex to avoid other threads change their running state
		//   before the conditipn checked and then reset state and notify all before 
		//   main goes into wait and receive the signals.
		// III.
		//  Locked on any conditions because the optimal way to sync is use single mutex.
		ck_pthread::mutex sync_mutex;
		
	
		// Set to 1 if lock is requested by someone
		bool lock_requested = 0;
	
		// Points to the current ckthread
		// Assigned when thread is being spawned via 
		// spawn_thread or creation of GIL
		static thread_local ckthread* current_thread_ptr;
		
		// Assigned on thread creation. Thread-local instance of executer.
		static thread_local ck_executer* executer;
	
		// Pointer to itself
		// Assigned when thread is being spawned via 
		// spawn_thread or creation of GIL
		static GIL* gil_instance;
		
		// Instance of Garbage Collector
		GC* gc;
	
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
		
		inline const std::vector<ckthread*>& get_threads() {
			return threads;
		};
		
		inline ck_pthread::cond_var& sync_var() {
			return sync_condition;
		};
		
		inline ck_pthread::mutex& sync_mtx() {
			return sync_mutex;
		};
		
		inline bool is_lock_requested() {
			return lock_requested;
		};
		
		
		// Performs lock of the sync_mutex.
		// Thread can me locked if the mutex in use, 
		//  so thread is being marked as locked.
		// After thread acquires the lock it is unmarked from 
		//  the locked state and could operate with this mutex.
		inline void lock() {
			current_thread()->set_locked(1);
			
			// Notify all threads that this thread is locked.
			// Used to notify operator thread in case when all
			//  other threads are locked on some mutex and no 
			//  notification signals are send.
			notify();
			sync_mutex.lock();
			
			current_thread()->set_locked(0);
		};
		
		// Attempts to lock the mutex. Returns 1 on success.
		inline bool try_lock() {
			return sync_mutex.try_lock();
		};
		
		// Performs unlocking of the mutex. 
		// If mutex was not locked or the error ocurred, 
		//  returns 0.
		inline bool unlock() {
			sync_mutex.unlock();
			
			// To avoid the situation when other threads waiting for some shit
			//  on sync_condition, ping them all.
			notify();
		};
		
		// Called to make other threads lock themselves 
		//  and wait for unlock on conditional variable.
		inline void request_lock() {
			if (current_thread()->own_gil || lock_requested)
				return;
			
			// Request sync_lock
			lock();
			
			// Mark this thread indicate ownership of the GIL lock.
			current_thread()->own_gil = 1;
			
			// Mutex will be locked till thread is not waiting.
			// Requesting all threads to lock
			lock_requested = 1;
			
			// Wait for other threads to pause by indicating lock_requested
			sync_condition.wait(sync_mutex, []() -> bool {
				// Mutex is locked here, so no threads can dispose or create befure it is released.
				
				bool all_locked = GIL::instance()->get_threads().size();
				for (int i = 0; i < GIL::instance()->get_threads().size(); ++i)
					if (GIL::instance()->get_threads()[i] != current_thread() && !GIL::instance()->get_threads()[i]->locked_state()) {
						all_locked = 0;
						break;
					}
					
				return all_locked;
			});
			
			// Here thread owns unique lock beyond other threads.
		};
		
		inline bool try_request_lock() {
			if (current_thread()->own_gil || lock_requested)
				return 0;
			
			// Request sync_lock
			if (!try_lock())
				return 0;
			
			// Mark this thread indicate ownership of the GIL lock.
			current_thread()->own_gil = 1;
			
			// Mutex will be locked till thread is not waiting.
			// Requesting all threads to lock
			lock_requested = 1;
			
			// Wait for other threads to pause by indicating lock_requested
			sync_condition.wait(sync_mutex, []() -> bool {
				// Mutex is locked here, so no threads can dispose or create befure it is released.
				
				bool all_locked = GIL::instance()->get_threads().size();
				for (int i = 0; i < GIL::instance()->get_threads().size(); ++i)
					if (GIL::instance()->get_threads()[i] != current_thread() && !GIL::instance()->get_threads()[i]->locked_state()) {
						all_locked = 0;
						break;
					}
					
				return all_locked;
			});
			
			// Here thread owns unique lock beyond other threads.
			return 1;
		};
		
		// Detaching lock and notifying all other threads that 
		//  may do some shit when waiting on the condition.
		inline void dequest_lock() {
			if (!current_thread()->own_gil || !lock_requested)
				return;
			
			current_thread()->own_gil = 0;
			lock_requested = 0;
			notify();
			unlock();
		};
		
		// Send notify to all threads.
		inline void notify() {
			sync_condition.notify_all();
		};
		
		// Stops all threads
		inline void stop() {
			for (int i = 0; i < threads.size(); ++i)
				threads[i]->set_running(0);
		}
		
		// Checks if some threads was requesting for a lock and locks.
		inline void accept_lock() {
			if (lock_requested) {
				// Wait till operator goes to sleep
				lock();
				
				// After that change the flag and go to wait
				current_thread()->set_locked(1);
				
				sync_condition.wait(sync_mutex, []() -> bool {
					return !GIL::instance()->is_lock_requested();
				});
				
				// Reset da flsg
				current_thread()->set_locked(0);
				
				// Release the mutex and go away
				unlock();
			}
		};
		
		// Mark this thread operating with IO and may block.
		inline void io_block() {
			current_thread()->set_blocked(1);
		};
		
		// Marks this thread as not operating with IO any more.
		// Bacaue one of thre threads could locked the others, this thread
		//  need to accept lock if it was requested.
		inline void io_unblock() {
			current_thread()->set_blocked(0);
			accept_lock();
		};
		
		
		// Spawns a new thread and registers it in GIL::threads.
		// On start, GIL global instance is assigned to it and then 
		// body function is called.
		// After body finishes, thread is being removed from threads list.
		void spawn_thread(std::function<void ()> body);
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