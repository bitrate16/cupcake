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
#include "lock_queue.h"
#include "exceptions.h"

using namespace std;
using namespace ck_core;
using namespace ck_exceptions;


// Init with nothing
thread_local ckthread*         GIL::current_thread_ptr = nullptr;
GIL*                           GIL::gil_instance       = nullptr;
thread_local ck_executer*      GIL::executer           = nullptr;
uint64_t                  ckthread::thread_counter     = 0;


GIL::GIL() { // : sync_lock(sync_mutex, sync_condition) {
	// Assign self instance
	GIL::gil_instance = this;
	
	// No lock needed. First start.
	// Allocate pointer to the new ckthread
	current_thread_ptr = new ckthread();
	threads.push_back(current_thread_ptr);
	
	GIL::gc = new GC();
	GIL::executer = new ck_executer();
	
	// Current (creator) thread is being tracked as primary thread.
	// This thread mush override all system signals and woke System.sysint(code)
	// in the current thread and executer.
	// When current thread finishes befure other threads it has to start waiting 
	// for other threads to finish their work. Current thread must lock on sync_condition
	// and every woke up check if there is any other threads.
	
	// All this ^ actions is done from main().
};

GIL::~GIL() {
	// Program termination.
	// Waiting till everything is completely dead.
	// Prevent creating new threads.
	
	// Logically there are no threads left.
	// ck_pthread::mutex_lock lock(sync_lock);
	
	// At this moment ignoring all signals from OS.
	sigset_t set;
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	
	// Derstroy all ck_threads (delete instances)
	// Causing undefined behavior on accidental delete of GIL.
	//  (but i don't care)
	// for (int i = 0; i < threads.size(); ++i) 
	//	delete threads[i];
	// Delete thread0
	
	delete executer; // should be disposed before gc because contains executer_stack_marker
	// Call last garbage collection
	gc->dispose();
	delete gc; // Press F to pay respects..
	
	// Delete instance of main thread
	delete threads[0];
};

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
	GIL::gil_instance = args->gil;
	GIL::executer     = new ck_core::ck_executer();
	GIL::current_thread_ptr = args->thread;
	
	// Wait for parent thread to finish initialization
	GIL::instance()->lock();
	
	// Keep lock acquired to allow body do some thread-unsafe oeprations within safe context.
	// GIL::instance()->unlock();
	
	// Copy id of thread
	uint64_t ctid = GIL::current_thread()->get_id();
	
	GIL::current_thread_ptr->set_running(1);
	// Call passed lambda
	args->body();
	
	// Dispose instance of thread if it still exist
	GIL::instance()->lock();
	
	GIL::current_thread_ptr->set_running(0);
	auto& threads = GIL::instance()->threads;
	for (int i = 0; i < threads.size(); ++i)
		if (threads[i]->get_id() == ctid) {
			threads.erase(threads.begin() + i);
			break;
		}
	
	GIL::instance()->unlock();
	
	// Dispose used values
	delete GIL::executer;
	delete args->thread;
	delete args;
	
	return 0;
};

