#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "GIL"

void handle_signal(int signal) {
	// Handling signals and process in separate thread.
	
};

int main(int argc, char **argv) {
	// 1. Initialize GIL:
	// --- Set up thread manager
	// --- Set up global context
	// --- Set up Garbage Collector
	// 2. Current thread assigns as first thread
	// 3. Create Thread Context
	// --- Create Root Scope
	// ---- Put default objects
	// --- Assign Root file path
	// --- Assign Root environment path
	// 4. Parse input source code into Representable Structure (RS)
	// --- Handle all error messages & output them as fault
	// 5. Execute (RS) in Executer instance via exec()
	
	// Executer recursive stach handling:
	// ------------------------------- <-- exec()
	// |                             |
	// |  BLOCK  OF CURRENT PROGRAM  |
	// |                             |
	// ------------------------------- <-- exec()
	// |       STACK DELIMITER       |
	// -------------------------------
	// |                             |
	// |    BLOCK OF NEXT PROGRAM    |
	// |                             |
	// - - - - - - - - - - - - - - - -
	// |                             |
	
	// Multithread pause:
	// 1. lock GIL.lock for access.
	// 2. mark all threads to pause
	// 3. each thread checks for pause request on any iteration
	// 4. locker thread waits for every thread to pause/lock
	// 5. operation in single thread mode
	// 6. send each thread unpause signal
	
	// Garbage collection:
	// 1. Lock Garbage Collector
	// 2. Execute task in multithreaded lock
	// 3. Collect garbage
	// 4. Exit multithreaded lock
	
	// Three main lockable contexts:
	// 1. GIL = lock queue.
	// 2. Object own = lock queue.
	// 3. IO = simple IO interrupt notification lock.
	// Use queue for locking:
	// queue::lock(func1, func2, func3):
	// 1. lock mutex2
	// 2. if queue is empty - call func3
	//      else call func1
	// 3. put self into queue
	// 4. unlock mutex2
	// 5. condition_variable.wait(mutex1, condition = { queue.first = this thread })
	// 6. if condition passed
	//      call func2
	// 7. exit queue::lock, other threads are waiting for this thread to remove itself from queue
	// queue::unlock(func):
	// 1. lock mutex2
	// 2. remove self from queue
	// 3. unlock mutex2
	// 4. call func
	// exit queu::unlock, forces other threads to handle next in the queue 
	
	gil = new ck_core::GIL();
	
	// Do some shit
	
	
	
	// Terminate
	std::unique_lock<std::mutex> threads_lock(gil->threads_lock);
	ck_core::ck_thread *self = gil->current_ckthread_noexcept();
	if (self == nullptr) {
		cout << "ARE YOU AHUEL TAM?" << endl;
		cout << "UNRESOLVED CORE ERROR" << endl;
		exit(1);
	}
	self->is_alive = 0;
	threads_lock.unlock();
	
	// Wait for all threads to terminate.
	std::unique_lock<std::mutex> term_lock(gil->sync_mutex);
	gil->sync_condition.wait(term_lock, [&] {
		std::unique_lock<std::mutex> threads_lock(gil->threads_lock);
		for (int i = 0; i < gil->threads.size(); ++i)
			if (gil->threads[i]->is_alive)
				return 0;
		return 1;
	});
	
	// Dispose GIL and 42.
	delete GIL;
};