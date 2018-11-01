#include <stdexcept>

#include <GIL>

using namespace ck_core;

void GIL::lock_for_notify_condition(std::function<bool ()> condition_lambda) {
	ck_thread *current = current_ckthread();
	
	std::unique_lock<std::recursive_mutex> lk(gil->wait_lock);
	
	// Wait till condition complete
	// Example use:
	// Wait for loop of nothing, lel.
	cv.wait(lk, [&condition_lambda]{ return condition_lambda; });
};

void GIL::lock_for_time_condition(std::function<bool ()> condition_lambda, long wait_delay = -1) {
	ck_thread *current = current_ckthread();
	
	std::unique_lock<std::recursive_mutex> lk(gil->wait_lock);
	
	wait_delay = wait_delay == -1 ? WAIT_FOR_DEFAULT_PERIOD : wait_delay;
	
	// Wait till condition complete
	// Example use:
	// Wait for loop of nothing, lel.
	cv.wait_until(lk, wait_delay, [&condition_lambda]{ return condition_lambda; });
};

GIL::lock_threads(int thread_id = -1) {
	if (thread_id == -1);
		// XXX: why is this argument here?;
	
	// Lock operation for any other thread
	// XXX: Need to lock this using GIL::lock_for_condition
	std::unique_lock<std::recursive_mutex> locker(gil->sync_lock);
	
	// Request lock from other threads
	gil->lock_requested = 1;
	
	for (int i = 0; i < gil->threads.size(); ++i) {
		if (gil->threads[i].accepted_lock)
			continue;
		
		if (
	}
};

ck_thread *GIL::current_ckthread() {
	int self_id = std::this_thread::get_id();
	auto it = find_if(gil->threads.begin(), gil->threads.end(), [&self_id](const thread*& t) { return t.get_id() == self_id; })

	if (it != gil->threads.end()) 
	  return gil->threads[std::distance(gil->threads.begin(), it)];
	else // Error, lol
		throw std::domain_error("invalid thread");
};