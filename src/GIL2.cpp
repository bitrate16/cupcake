#include "GIL2.h"

#include <thread>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <csignal>

#include "executer.h"
#include "exceptions.h"
#include "vobject.h"
#include "GC.h"
#include "exceptions.h"

using namespace std;
using namespace ck_core;
using namespace ck_exceptions;


// Init with nothing
thread_local gil_thread*       GIL::current_thread_ptr = nullptr;
GIL*                           GIL::gil_instance       = nullptr;
thread_local ck_executer*      GIL::executer           = nullptr;
std::atomic<uint64_t>   gil_thread::thread_counter     = 0;


// GIL constructor is called on main() call.
// Default constructor mapping current thread to 
//  it's descriptor & saves into GIL::threads
// Creates & attaches instance of exeuter for this thread.
// Creates global instance of GC and attachs it to GIL.
// Maps static access variable instance to itself.
GIL::GIL() { // : sync_lock(sync_mutex, sync_condition) {
	// Assign instance of GIL to be accessible 
	//  from any point of program
	GIL::gil_instance = this;
	
	// Create descriptor for current thread & store it as Thread0
	current_thread_ptr = new gil_thread();
	threads.push_back(current_thread_ptr); // no lock needed
	
	// Create GC instance before any object is created
	GIL::gc = new GC();
	// Create executer instance mapped to current Thread0
	GIL::executer = new ck_executer();
	
	// Current (creator) thread is being tracked as primary thread.
	// This thread mush override all system signals and woke System.sysint(code)
	// in the current thread and executer.
	// When current thread finishes befure other threads it has to start waiting 
	// for other threads to finish their work. Current thread must lock on sync_condition
	// and every woke up check if there is any other threads.
	
	// All this ^ actions is done from main().
};

// At first, blocks all system signals to give GC a 
//  chance to collect all left objects.
// Secondly, calls GC::dispose & deletes GC.
// Finally unbinds current thread deskriptor from GIL & returns.
GIL::~GIL() {
	// Program termination.
	// Waiting till everything is completely dead.
	// Prevent creating new threads.
	
	// Logically there are no threads left.
	
	// At this moment ignoring all signals from OS.
	
	// XXX: Platform-dependent code
	// Make program ignore all signals from OS.
	sigset_t set;
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	
	// Derstroy all ck_threads (delete instances)
	// Causing undefined behavior on accidental delete of GIL.
	//  (but i don't care)
	// for (int i = 0; i < threads.size(); ++i) 
	//	delete threads[i];
	// Delete thread0
	
	// Delete executer instance
	delete executer; // should be disposed before gc because contains executer_stack_marker
	// Call last garbage collection
	// Dispose all objects on GC & delete GC
	gc->dispose();
	delete gc; // Press F to pay respects..
	
	// Delete instance of main thread
	
	// Delete descriptor of Main thread
	delete current_thread_ptr;
};

// Called on new stack created
void thread_stack_wrapper(int argc, void** argv) {
	// argv ~ thread_spawner_args
	thread_spawner_args* args = (thread_spawner_args*) ((thread_spawner_args**) argv) [0];
	
	try {
		args->body();
	} catch (...) {}
}

// Dummy wrapper for newly created thread.
// Set ups signal handlers for this thread so it will not react to any signal.
//  Instead, signal should be redirected to Main thread.
// GIL attaches self instance to current GIL::gil_instance.
// GIL binds thread descriptor to current thread.
// Creates new executer for current thread.
// GIL Marks this thread as running
// After GIL have to call passed function body 
//  and handle all exceptions thrown by it.
//  Called body unction has privilleged mode with 
//   GIL global lock acquired.
// Finally GIL have to lock again, mark thread as dead, 
//  remove it from list of threads & delete.
void ck_core::thread_spawn_wrapper(ck_core::thread_spawner_args* args) {	
	
	// L O C K _ P A R E N T
	
	args->init_sync.lock();
	args->init_state = 1;
	
	
	// S I G N A L _ I N I T
	
	// XXX: Platform-dependent code
	// Set to ignore all signals to make main thread catch them instead
	sigset_t set;
	sigfillset(&set);
	if (pthread_sigmask(SIG_BLOCK, &set, NULL)) 
		return;
	
	
	// T H R E A D _ I N I T
	
	// Append current thread to threads vector
	{	
		// For thread creation
		std::unique_lock<std::recursive_mutex> lk(GIL::instance()->threads_mtx());
		
		// Create new gil_thread pointing to std::thread
		GIL::current_thread_ptr = new ck_core::gil_thread(args->thread);	
		GIL::instance()->threads.push_back(GIL::current_thread_ptr);
	}
	
	// Copy pointers to GIL values
	GIL::gil_instance = args->gil; // unused, static
	GIL::executer     = new ck_core::ck_executer();
	
	// Copy id of thread, requires thread-safe
	int64_t ctid = GIL::current_thread()->get_id();
	
	// Erase stack replacement
	ck_core::stack_locator::erase_all();
	
	
	// U N L O C K _ P A R E N T
	
	args->init_sync.unlock();
	args->init_state = 0;
	args->init_sync_var.notify_all();
	
	
	// T H R E A D _ R U N
	
	// Call passed lambda.
	// GIL is locked during call
	// Call new thread on new stack
	if (!ck_core::stack_locator::call_replace_stack(1, (void**) &args, &thread_stack_wrapper, args->thread_stack_size))
	
	// D I S P O S E _ T H R E A D
	
	// Dispose instance of thread if it still exist
	GIL::instance()->lock();
	
	// Dispose used values
	delete GIL::executer;
	delete args;
	
	// Mark thread as dead
	GIL::current_thread_ptr->set_running(0);
	
	// Lock to delete	
	std::unique_lock<std::recursive_mutex> lk(GIL::instance()->threads_mtx());
	
	// Remove this thread from list of threads
	auto& threads = GIL::instance()->threads;
	for (int i = 0; i < threads.size(); ++i)
		if (threads[i]->get_id() == ctid) {
			threads.erase(threads.begin() + i);
			break;
		}
	
	// Release GIL to breath free
	GIL::instance()->unlock_no_accept();
};

