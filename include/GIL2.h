#pragma once

#include <thread>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "exceptions.h"
#include "ck_pthread.h"


namespace ck_core {
	
	class ckthread {
		friend class GIL;
		
		// Indicates that this thread has native thread intance attached
		bool has_native = 0;
		// Contains instance of the native thread
		ck_pthread::thread* native_thread = nullptr;
		// Contains native thread integer id for faster search over threads list
		uint64_t native_thread_id = 0;
		// Total thread amount counter (For debug)
		static uint64_t thread_counter;
		// Thread descriptor id
		uint64_t thread_id;
		
		// Thread state is described by values below.
		//  is running is equal to 1 when thread is running and is sef to 0, thread must instantly stops.
		//  is locked set to 1 when thread allowed GIL to acquire lock on it.
		//  is blocked is set to 1 when thread may perform IO operations and GIL can acquire lock on it.
		
		bool running = 1;
		bool blocked = 0;
		bool locked  = 0;
		
		// Indicates if this thread is owning GIL lock now.
		// Used to avoid situuations of incorrect GIL locking/unlocking.
		bool own_gil = 0;
		
		// Default constructor,
		//  Used in GIL constructor to attach descriptor of current thread.
		//  Used on spawning new thread and GIL manually adding all fields into it.
		ckthread() : native_thread_id(ck_pthread::thread::this_thread().get_id()) { thread_id = thread_counter++; };
		
	public:
		
		// Destructor have to dispose thread instance by itself.
		// Aka safe constructor for newly created ck_pthread object.
		~ckthread() {
			// native_thread is not null if it was created by GIL::spawn_thread
			// Logically ckthread is disposed with native_thread
			
			// Delete ck_pthread instance here, because thread is stopping right now
			if (has_native)
				delete native_thread;
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
		
		// Returns id of ck_pthread
		inline uint64_t get_native_id() {
			return native_thread_id;
		};
	
		// Set blocked = locked = 0
		inline void clear_blocks() {
			blocked = locked = 0;
		};
		
		// Total threads count
		inline static uint64_t count() {
			return ckthread::thread_counter;
		};
		
		// Returns id of this ckthread 
		inline uint64_t get_id() {
			return thread_id;
		};
		
		// Resets state of the thread to the default
		inline void restate() {
			running = 1;
			locked  = 0;
			blocked = 0;
		};
	};
	
	// Forward declare
	class GC;
	class ck_executer;
	
	extern void* thread_spawn_wrapper(void* args);
	
	class GIL {

		friend void* thread_spawn_wrapper(void* args);
		
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
		ck_pthread::recursive_mutex sync_mutex;
		
		// Mutex used for thread vector synchromized access
		ck_pthread::recursive_mutex threads_mutex;
	
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
		
		// Structure with arguments of thread spawner, 
		//  used to pass GIL environment
		struct thread_spawner_args {
			GIL* gil;
			ckthread* thread;
			std::function<void ()> body;
		};
	
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
		
		inline ck_pthread::recursive_mutex& sync_mtx() {
			return sync_mutex;
		};
		
		// Returns threads_mutex. It has to be locked for safe threads list access
		inline ck_pthread::recursive_mutex& threads_mtx() {
			return threads_mutex;
		};
		
		inline bool is_lock_requested() {
			return lock_requested;
		};
		
		// Find thread by thread_id
		// WARNING: This call is not thread-safe, meaning thread 
		//  should request GIL lock before call this function.
		inline static ckthread* thread_by_id(int thread_id) {
			GIL* gil = GIL::instance();
			
			ck_pthread::mutex_lock lk(GIL::instance()->threads_mutex);
			
			for (int i = 0; i < gil->threads.size(); ++i)
				if (gil->threads[i]->get_id() == thread_id)
					return gil->threads[i];
			
			return nullptr;
		};
		
		
		// Performs lock of the sync_mutex.
		// Thread can be locked if the mutex in use, 
		//  so thread is being marked as locked.
		// After thread acquires the lock it is unmarked from 
		//  the locked state and could operate with this mutex.
		inline void lock() {
		#ifndef CK_SINGLETHREAD
			current_thread()->set_locked(1);
			
			// Notify all threads that this thread is locked.
			// Used to notify operator thread in case when all
			//  other threads are locked on some mutex and no 
			//  notification signals are send.
			notify();
			sync_mutex.lock();
			
			current_thread()->set_locked(0);
		#endif
		};
		
		// Attempts to lock the mutex. Returns 1 on success.
		inline bool try_lock() {
		#ifndef CK_SINGLETHREAD			
			notify();
			return sync_mutex.try_lock();
		#else
			return 1;
		#endif
		};
		
		// Performs unlocking of the mutex. 
		// If mutex was not locked or the error ocurred, 
		//  returns 0.
		inline bool unlock() {
		#ifndef CK_SINGLETHREAD
			bool res = sync_mutex.unlock();
			
			// To avoid the situation when other threads waiting for some shit
			//  on sync_condition, ping them all.
			notify();
			accept_lock();
			
			return res;
		#endif
			return 0;
		};
		
		// Performs unlocking of the mutex. 
		// If mutex was not locked or the error ocurred, 
		//  returns 0.
		inline bool unlock_no_accept() {
		#ifndef CK_SINGLETHREAD
			bool res = sync_mutex.unlock();
			
			// To avoid the situation when other threads waiting for some shit
			//  on sync_condition, ping them all.
			notify();
			
			return res;
		#endif
			return 0;
		};
		
		// Called by thread to acquire lock on sync_mutex and 
		//  notify other threads to lock by setting lock_requestd to 1.
		inline void request_lock() {
		#ifndef CK_SINGLETHREAD
		
			if (current_thread()->own_gil || lock_requested)
				return;
			
			// Request sync_lock
			lock();
			
			// Mark this thread indicate ownership of the GIL lock.
			current_thread()->own_gil = 1;
			
			// Mutex will be locked till thread is not waiting.
			// Requesting all threads to lock
			lock_requested = 1;
			
			current_thread()->set_locked(1);
			
			notify();
			
			// Wait for other threads to pause by indicating lock_requested
			sync_condition.wait(sync_mutex, []() -> bool {
				// Mutex is locked here, so no threads can dispose or create before it is released.
			
				ck_pthread::mutex_lock lk(GIL::instance()->threads_mutex);
				
				bool all_locked = 1;
				if (GIL::instance()->get_threads().size() > 1) {
					for (int i = 0; i < GIL::instance()->get_threads().size() && !all_locked; ++i)
						if (GIL::instance()->get_threads()[i] != current_thread() && !GIL::instance()->get_threads()[i]->locked_state()) {
							all_locked = 0;
							break;
						}
				}
				
				return all_locked;
			});
			
			current_thread()->set_locked(0);
			
			// Here thread owns unique lock beyond other threads.
		#endif
		};
		
		inline bool try_request_lock() {
		#ifndef CK_SINGLETHREAD
			
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
			
			current_thread()->set_locked(1);
			
			notify();
			
			// Wait for other threads to pause by indicating lock_requested
			sync_condition.wait(sync_mutex, []() -> bool {
				// Mutex is locked here, so no threads can dispose or create befure it is released.
			
				ck_pthread::mutex_lock lk(GIL::instance()->threads_mutex);
				
				bool all_locked = 1;
				if (GIL::instance()->get_threads().size() > 1) {
					for (int i = 0; i < GIL::instance()->get_threads().size(); ++i) 
						if (GIL::instance()->get_threads()[i]->get_id() != current_thread()->get_id() && !GIL::instance()->get_threads()[i]->locked_state()) {
							all_locked = 0;
							break;
						}
				}
					
				return all_locked;
			});
			
			current_thread()->set_locked(0);
			
			// Here thread owns unique lock over other threads.
		#endif
			return 1;
		};
		
		// Detaching lock and notifying all other threads that 
		//  may do some shit when waiting on the condition.
		inline void dequest_lock() {
		#ifndef CK_SINGLETHREAD
			if (!current_thread()->own_gil || !lock_requested)
				return;
			
			current_thread()->own_gil = 0;
			lock_requested = 0;
			unlock();
		#endif
		};
		
		// Send notify to all threads.
		inline void notify() {
		#ifndef CK_SINGLETHREAD
			sync_condition.notify_all();
		#endif
		};
		
		// Stops all threads
		inline void stop() {
		#ifndef CK_SINGLETHREAD
			
			// Unsafe ares that can be called from sgnal processor 
			//  or any other part that does allow gil lock
			
			ck_pthread::mutex_lock lk(threads_mutex);
			
			for (int i = 0; i < threads.size(); ++i)
				threads[i]->set_running(0);
		#endif
		}
		
		// Checks if some threads was requesting for a lock and locks.
		inline void accept_lock() {
		#ifndef CK_SINGLETHREAD
			if (is_lock_requested()) {
				// Wait on mutex for lock for next wait
				lock();
				
				// Change the flag and go to wait
				current_thread()->set_locked(1);
				
				// Wake waiter
				notify();
				
				// Wait for lock to be dequested
				sync_condition.wait(sync_mutex, []() -> bool {
					return !GIL::instance()->is_lock_requested();
				});
				
				// Reset da flag
				current_thread()->set_locked(0);
				
				// Release the mutex and go away
				unlock();
			}
		#endif
		};
		
		// Mark this thread operating with IO and may block.
		inline void io_block() {
		#ifndef CK_SINGLETHREAD
			current_thread()->set_blocked(1);
			notify();
		#endif
		};
		
		// Marks this thread as not operating with IO any more.
		// Bacaue one of thre threads could locked the others, this thread
		//  need to accept lock if it was requested.
		inline void io_unblock() {
		#ifndef CK_SINGLETHREAD
			current_thread()->set_blocked(0);
			accept_lock();
		#endif
		};
		
		
		// Spawns a new thread and registers it in GIL::threads.
		// On start, GIL global instance is assigned to it and then 
		// body function is called.
		// After body finishes, thread is being removed from threads list.
		// Returns id of created thread
		// After call was performed and successfully complete, GIL lock is 
		//  still locked by created thread.
		uint64_t spawn_thread(std::function<void ()> body) {
		#ifndef CK_SINGLETHREAD
			ckthread* th = new ckthread();
			ck_pthread::thread* nt;
			
			// Lock in here to prevent uninitialized value acccess if thread finishes 
			//  before it was attached and delached from GIL threads array
			GIL::instance()->lock();
			
			// For thread creation
			ck_pthread::mutex_lock lk(threads_mutex);
			
			thread_spawner_args* args = new thread_spawner_args;
			args->gil    = this;
			args->thread = th;
			args->body   = body;
			
			// XXX: Custom thread stack size
			// Create thread descriptor, by default tread is running & not locked to prevent GC.
			int retnum;
			nt = new ck_pthread::thread(ck_core::thread_spawn_wrapper, args, &retnum);
			
			// Failed to create thread
			if (retnum) {
				delete args;
				delete nt;
				delete th;
				
				return -1;
			}
			
			nt->detach();
			
			// Assign to descriptor
			th->native_thread    = nt;
			th->native_thread_id = nt->get_id();
			th->has_native       = 1;
			
			threads.push_back(th);
			
			uint64_t tid = th->get_id();
			
			GIL::instance()->unlock();
			
			return tid;
		#else
			return -1;
		#endif
		};
	};

	// Class used to produce GIL lock during function call
	// GIL::lock() is called on creation,
	// GIL::unlock() is called on destruction of object.
	class GIL_lock {
	public:
		
		GIL_lock() {
			GIL::instance()->lock();
		};
		
		~GIL_lock() {
			GIL::instance()->unlock();
		};
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