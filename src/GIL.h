#pragma once

#include <thread>
#include <vector>

#include "GC"

namespace ck_core {	
	/*
	 * Contains all information about thread used by interpreter.
	 * Information if user for thread synchronization and interrupts.
	 */
	class ck_thread {
		public:
		// Pointer to the assigned thread
		std::thread *thread = nullptr;
		// Unique thread number in ck debug
		int thread_id       = -1;
		// Thread state on ck side
		int state           = -1;
		// Interrupt code handling
		int interrupt       = -1;
		// Set to 1 while thread is performing blocking operation
		// (aka read/write, listed to input or sockets)
		// is_blocked used for safety pausing threads while they are freezed.
		// (aka cin >> variable => is_blocked = 1; GIL.lock_threads can pause this thread)
		bool is_blocked     = 0;
		// Set to 1 if thread is locked on some object.
		bool is_object_lock = 0;
	};
	
	// GIL lock_threads mutex used for preventing multiple threads from making locks at same time.
	std::mutex GIL_lock_threads_mutex;
	
	/*
	 * Global Interpreter Lock type
	 * Single for each thread, provides global synchronization, thread control,
	 * garbage collector access.
	 */
	class GIL {
		public:
		std::mutex GIL_lock;
		// List of all spawned threads
		std::vector<ck_thread> threads;
		// Garbage collector
		GC gc;
		// Lock signal flag. Equals to 1 if threads expected to pause.
		int lock_requested = 0;
		
		GIL();
		~GIL();
		
		// Locks all threads but current.
		// Waits till all threads will receive lock signal and pause.
		// Returns 1 if lock has been successfull, 0 else.
		bool lock_threads(int thread_id = -1);
		
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
	};
	
	// Instance of GIL for each thread.
	static GIL *gil;
};