#include "GIL2"
#include "GC"

using namespace ck_core;

GIL::GIL() {
	// Assing self instance
	gil = this;
	
	// No lock needed. First start.
	ckthread *ct = new ckthread();
	threads->push(ct);
};

// Ignore for signals
static void dummy_signal(int signal) {
	signal(SIGINT,  dummy_signal); // <-- user interrupt
	signal(SIGTERM, dummy_signal); // <-- Terminate request
	signal(SIGABRT, dummy_signal); // <-- abortion is murder
};

GIL::~GIL() {
	// Program termination.
	// Waiting till everything is completely dead.
	// Prevent creating new threads.
	std::unique_lock<std::mutex> lock(threads_lock);
	
	// At this moment ignoring all signals from OS.
	signal(SIGINT,  dummy_signal); // <-- user interrupt
	signal(SIGTERM, dummy_signal); // <-- Terminate request
	signal(SIGABRT, dummy_signal); // <-- abortion is murder
	
	// Derstroy all ck_threads (delete instances)
	for (int i = 0; i < threads.size(); ++i) {
		delete threads[i]->thread;
		delete threads[i];
	}
	
	// Call last garbage collection
	GC.dispose();
	
	// Finally commit suicide
	delete this;
};