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
thread_local ckthread*    GIL::current_thread_ptr = nullptr;
GIL*                      GIL::gil_instance       = nullptr;
thread_local ck_executer* GIL::executer           = nullptr;


GIL::GIL() { // : sync_lock(sync_mutex, sync_condition) {
	// Assign self instance
	GIL::gil_instance = this;
	GIL::current_thread();
	GIL::gc = new GC();
	GIL::executer = new ck_executer();
	
	// No lock needed. First start.
	// Allocate pointer to the new ckthread
	current_thread_ptr = new ckthread();
	threads.push_back(current_thread_ptr);
	
	// Current (creator) thread is being tracked as primary thread.
	// This thread mush override all system signals and woke System.sysint(code)
	// in the current thread and executer.
	// When current thread finishes befure other threads it has to start waiting 
	// for other threads to finish their work. Current thread must lock on sync_condition
	// and every woke up check if there is any other threads.
	
	// All this ^ actions is done from main().
};

// Ignore for signals
static void dummy_signal(int sig) {
	signal(SIGINT,  dummy_signal); // <-- user interrupt
	signal(SIGKILL, dummy_signal); // <-- kill signal
	signal(SIGTERM, dummy_signal); // <-- Terminate request
	signal(SIGABRT, dummy_signal); // <-- abortion is murder
};

GIL::~GIL() {
	// Program termination.
	// Waiting till everything is completely dead.
	// Prevent creating new threads.
	
	// Logically there are no threads left.
	// ck_pthread::mutex_lock lock(sync_lock);
	
	// At this moment ignoring all signals from OS.
	dummy_signal(-1);
	
	// Derstroy all ck_threads (delete instances)
	// Causing undefined behavior on accidental delete of GIL.
	//  (but i don't care)
	for (int i = 0; i < threads.size(); ++i) 
		delete threads[i];
	
	// Call last garbage collection
	gc->dispose();
	
	delete gc; // Press F to pay respects..
	delete executer;
};
