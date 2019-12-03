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
thread_local ckthread*         GIL::current_thread_ptr = nullptr;
GIL*                           GIL::gil_instance       = nullptr;
thread_local ck_executer*      GIL::executer           = nullptr;
uint64_t                  ckthread::thread_counter     = 0;


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
	current_thread_ptr = new ckthread();
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
	// ck_pthread::mutex_lock lock(sync_lock);
	
	// At this moment ignoring all signals from OS.
	
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
	delete threads[0];
};

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
void* ck_core::thread_spawn_wrapper(void* argv) {
	// argv points to GIL::thread_spawner_args
	
	if (!argv)
		return 0;
	
	// Set to ignore all signals to make main thread catch them instead
	sigset_t set;
	sigfillset(&set);
	if (pthread_sigmask(SIG_BLOCK, &set, NULL)) 
		return 0;
	
	GIL::thread_spawner_args* args = static_cast<GIL::thread_spawner_args*>(argv);
	// Copy pointers to GIL values
	GIL::gil_instance = args->gil; // unused, static
	GIL::current_thread_ptr = args->thread;	
	
	// Wait for parent thread to finish initialization
	// GC can not be called because this thread is not marked locked.
	// GIL::instance()->lock(); // unused because GC will not call
	
	GIL::executer     = new ck_core::ck_executer();
	
	// Copy id of thread, requires thread-safe
	uint64_t ctid = GIL::current_thread()->get_id();
	
	// Set state of this thread as running to indicate for executer that it can run now
	// GIL::current_thread_ptr->set_running(1); // Unused, because set by default
	
	// Call passed lambda
	args->body(); // XXX: Try-catch, unhandled only
	
	// Dispose instance of thread if it still exist
	GIL::instance()->lock();
	
	// Dispose used values
	delete GIL::executer;
	
	// Mark thread as dead
	GIL::current_thread_ptr->set_running(0);
	
	// Lock to delete	
	ck_pthread::mutex_lock lk(GIL::instance()->threads_mtx());
	
	// Remove this thread from list of threads
	auto& threads = GIL::instance()->threads;
	for (int i = 0; i < threads.size(); ++i)
		if (threads[i]->get_id() == ctid) {
			threads.erase(threads.begin() + i);
			break;
		}
		
	delete args->thread;
	delete args;
	
	// Release GIL to breath free
	GIL::instance()->unlock_no_accept();
	
	return 0;
};

