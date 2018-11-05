#include <stdexcept>

#include "GIL"

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


bool GIL::lock_threads(int thread_id = -1) {
	if (thread_id == -1);
		// XXX: why is this argument here?;
	
	// Lock this piece of shit and protect this context from being interrupted
	
	// When first thread will call this function, this lock will
	// be acquired by it and any thread that will attempt to notify it 
	// (at first, lock acquire this lock) will be blocked till 
	// operator thread (first) will not sit on the wait.
	// If operator will sit on waititng without condition being passed, 
	// second thread will unblock and insert itself into queue.
	// sync_lock::lock will call gil_sync::GIL_NOTIFY_SYNC_LOCK and 
	// notify operator to wake up. Then it will sit on wait
	// and third thread will unblock
	// ...
	// All thread that called GIL::lock_threads will fit in queue 
	// and finally operator thread will unblock. 
	// In other case, when condition is true, opertor will perform it's 
	// synchronized operations and then unblock the queue with GIL::unlock().
	
	// We have to lock this mutex before changing main lock flag.
	// Locking from there will allow other threads to stay in queue.
	// All block of this function is now lock-protected.
	// While other threads will join wait wueue on GIL::sync_mutex
	// and GIL::sync_condition, this thread will safety
	// pass to the wait and release the mutex, allowing other threads to join.
	std::unique_lock<std::mutex> lk(gil->sync_mutex);
	
	// Lock operation for any other thread
	// All waiting threads are now marked as accepted blocking. 
	// Also GIL_NOTIFY_SYNC_LOCK avoids situation when opeartor
	// thread computed all_locked value to be 0 and all threads 
	// shortly became blocked while operator thread didn't receive 
	// their state change and waits forever.
	gil->sync_lock.shared_lock(lk, gil_sync::GIL_NOTIFY_SYNC_LOCK, gil_sync::GIL_SYNC_UNLOCK);
	
	// Request lock from other threads
	gil->lock_requested = 1;
	
	// Check if all threads are blocked
	bool all_locked = 1;
	for (int i = 0; i < gil->threads.size(); ++i)
		if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked) && gil->threads[i]->is_alive)
			all_locked = 0;
			
	if (!all_locked) {
		// Wait till all threads will accept block
		// In this case there is more than one thread that wants to acquire the lock
		gil->sync_condition.wait(lk, [&]{ 
			for (int i = 0; i < gil->threads.size(); ++i)
				if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked) && gil->threads[i]->is_alive)
					return 0;
			return 1; 
		});
	}
	
	// We finally acquired the lock and can operate in synchronized singlethread mode.
	return 1;
};

bool GIL::try_lock_threads(int thread_id = -1) {
	if (thread_id == -1);
		// XXX: why is this argument here?;
	
	// Lock operation for any other thread
	// All waiting threads are now marked as accepted blocking. 
	// Also GIL_NOTIFY_SYNC_LOCK aoids situation when opeartor
	// thread computed all_locked value to be 0 and all threads 
	// shortly became blocked while operator thread didn't receive 
	// their state change and waits forever.
	std::unique_lock<std::mutex> lk(gil->sync_mutex);
	if (!gil->sync_lock.try_lock())
		return 0;
	
	// Request lock from other threads
	gil->lock_requested = 1;
	
	// Check if all threads are blocked
	bool all_locked = 1;
	for (int i = 0; i < gil->threads.size(); ++i)
		if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked) && gil->threads[i]->is_alive)
			all_locked = 0;
			
	if (!all_locked) {	
		// вошли сюда, все одновременно сработали, отправили уведомление, основной повиснет.
		// Wait till all threads will accept block
		// In this case there is more than one thread that wants to acquire the lock
		gil->sync_condition.wait(lk, [&]{
			for (int i = 0; i < gil->threads.size(); ++i)
				if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked) && gil->threads[i]->is_alive)
					return 0;
			return 1; 
		});
	}
	
	gil->sync_lock.unlock();
	
	return 1;
};

bool GIL::unlock_threads(int thread_id = -1) {
	gil->sync_lock.unlock();
};

static void thread_dummy(GIL *gil, std::function<void ()> &body) {
	// Request lock so if this thread still not added to GIL::threads.
	std::unique_lock<std::mutex> lock(gil->threads_lock, std::ref(body));
	ck_core::gil = gil;
	lock.unlock();
	
	// Call body
	body();
	
	// Lock this mutex again and remove itself from threads list.
	lock.lock();
	
	int self_id = std::this_thread::get_id();
	auto it = find_if(gil->threads.begin(), gil->threads.end(), [&self_id](const thread*& t) { return t.get_id() == self_id; })

	if (it != gil->threads.end()) {
		int i = std::distance(gil->threads.begin(), it);
		// delete gil->threads[i].thread;
		// gil->threads.erase(i);
		// Notify GIL that this thread is dead now.
		gil->threads[i]->is_alive = 0;
		gil->notify_sync_lock();
	} else
		throw std::domain_error("invalid thread");
};

void GIL::spawn_thread(std::function<void ()> body) {
	
	// Lock this mutex to prevent thread from starting before 
	// it is added to GIL::threads vector. 
	std::unique_lock<std::mutex> lock(gil->threads_lock);
	
	std::thread *t = new std::thread(thread_dummy, gil, std::ref(body));
	ck_thread *ct  = new ck_thread();
	ct->thread     = t;
	gil->threads->push(ct);
	t.detach();
};

ck_thread *GIL::current_ckthread() {
	int self_id = std::this_thread::get_id();
	auto it = find_if(gil->threads.begin(), gil->threads.end(), [&self_id](const thread*& t) { return t.get_id() == self_id; })

	if (it != gil->threads.end()) 
	  return gil->threads[std::distance(gil->threads.begin(), it)];
	else // Error, lol
		throw std::domain_error("invalid thread");
};

ck_thread *GIL::current_ckthread_noexcept() noexcept {
	int self_id = std::this_thread::get_id();
	auto it = find_if(gil->threads.begin(), gil->threads.end(), [&self_id](const thread*& t) { return t.get_id() == self_id; })

	if (it != gil->threads.end()) 
	  return gil->threads[std::distance(gil->threads.begin(), it)];
	else // Error, lol
		return nullptr;
};

void GIL::notify_sync_lock() {
	gil->sync_condition.notify_all();
};