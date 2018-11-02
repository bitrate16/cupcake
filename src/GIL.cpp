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

GIL::lock_threads(int thread_id = -1) {
	if (thread_id == -1);
		// XXX: why is this argument here?;
	
	// Lock operation for any other thread
	// All waiting threads are now marked as accepted blocking. 
	// Also GIL_NOTIFY_SYNC_LOCK aoids situation when opeartor
	// thread computed all_locked value to be 0 and all threads 
	// shortly became blocked while operator thread didn't receive 
	// their state change and waits forever.
	// все влетели, один прошел, остальные повисли, не успев выставить 1
	gil->sync_lock.lock(gil_sync::GIL_NOTIFY_SYNC_LOCK, gil_sync::GIL_SYNC_UNLOCK);
	
	// Request lock from other threads
	gil->lock_requested = 1;
	
	// Check if all threads are blocked
	bool all_locked = 1;
	for (int i = 0; i < gil->threads.size(); ++i)
		if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked))
			all_locked = 0;
			
	if (!all_locked) {
		// вошли сюда, все одновременно сработали, отправили уведомление, основной повиснет.
		// Wait till all threads will accept block
		// In this case there is more than one thread that wants to acquire the lock
		std::unique_lock<std::mutex> lk(gil->sync_mutex);
		gil->sync_condition.wait(lk, [&]{ // надо делать асинхронную проверку. Возможно, другим потоком проверять,
										  // поменяли ли они флаги. То есть нужно что-то, что пробдуит этот поток
										  // когда остальные остановятся.
										  // Два варианта: 
										  // 1. while(true) - но это херово, так как какой-то поток может, например, фризануться.
										  // 2. async - лучше, так как никто никого ждать не будет, но, тем не менее, так же херово
										  //            так как создается отдельный поток и в общем то плодить асинхронные ожидания - себя не любить.
										  // 3. какой-то триггер или очередь, которая примет все уведомления и, как только этот заблочится, отправит их.
										  // критическое решение: не больше одного потока могут завлодеть глобальным локом (очередью) try_lock_threads()
			for (int i = 0; i < gil->threads.size(); ++i)
				if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked))
					return 0;
			return 1; 
		});
	}
	
	gil->sync_lock.unlock();
};

GIL::try_lock_threads(int thread_id = -1) {
	if (thread_id == -1);
		// XXX: why is this argument here?;
	
	// Lock operation for any other thread
	// All waiting threads are now marked as accepted blocking. 
	// Also GIL_NOTIFY_SYNC_LOCK aoids situation when opeartor
	// thread computed all_locked value to be 0 and all threads 
	// shortly became blocked while operator thread didn't receive 
	// their state change and waits forever.
	// все влетели, один прошел, остальные повисли, не успев выставить 1
	gil->sync_lock.try_lock();
	
	// Request lock from other threads
	gil->lock_requested = 1;
	
	// Check if all threads are blocked
	bool all_locked = 1;
	for (int i = 0; i < gil->threads.size(); ++i)
		if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked))
			all_locked = 0;
			
	if (!all_locked) {
		
		
		// В этом случае аналогичная проблема, только будут пропущены уведомления от потоков, которые решили сами заблокироваться.
		// В итоге необходимо сделать дополнительный поток, который будет ждать, пока все потоки заблокируются, 
		// и только потом даст этому проработать.
		// Или непосредственно в этом потоке проверять готовность каждые N миллисекунд.
		
		
		// вошли сюда, все одновременно сработали, отправили уведомление, основной повиснет.
		// Wait till all threads will accept block
		// In this case there is more than one thread that wants to acquire the lock
		std::unique_lock<std::mutex> lk(gil->sync_mutex);
		gil->sync_condition.wait(lk, [&]{ // надо делать асинхронную проверку. Возможно, другим потоком проверять,
										  // поменяли ли они флаги. То есть нужно что-то, что пробдуит этот поток
										  // когда остальные остановятся.
										  // Два варианта: 
										  // 1. while(true) - но это херово, так как какой-то поток может, например, фризануться.
										  // 2. async - лучше, так как никто никого ждать не будет, но, тем не менее, так же херово
										  //            так как создается отдельный поток и в общем то плодить асинхронные ожидания - себя не любить.
										  // 3. какой-то триггер или очередь, которая примет все уведомления и, как только этот заблочится, отправит их.
										  // критическое решение: не больше одного потока могут завлодеть глобальным локом (очередью) try_lock_threads()
			for (int i = 0; i < gil->threads.size(); ++i)
				if (!(gil->threads[i]->thread->get_id() != std::this_thread::get_id()) && gil->threads[i]->is_blocked))
					return 0;
			return 1; 
		});
	}
	
	gil->sync_lock.unlock();
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