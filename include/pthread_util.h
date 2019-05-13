#pragma once

#include <pthread.h>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace ck_pthread {
	
	// Creates recursive mutex instance and returns a copy.
	static pthread_mutex_t create_recursive_mutex() {
		pthread_mutex_t mtx;
		pthread_mutexattr_t mtx_attr;

		pthread_mutexattr_init(&mtx_attr);
		pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&mtx, &mtx_attr);
		
		return mtx;
	};
	
	// Creates simple mutex instance and returns a copy.
	static pthread_mutex_t create_mutex() {
		pthread_mutex_t mtx;
		pthread_mutexattr_t mtx_attr;

		pthread_mutexattr_init(&mtx_attr);
		pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_NORMAL);
		pthread_mutex_init(&mtx, &mtx_attr);
		
		return mtx;
	};
	
	// Performs mutex lock
	static inline bool lock(pthread_mutex_t& mtx) {
		return !pthread_mutex_lock(&mtx);
	};
	
	// Performs mutex unlock
	static inline bool unlock(pthread_mutex_t& mtx) {
		return !pthread_mutex_unlock(&mtx);
	};
	
	// Performs mutex unlock
	static inline bool trylock(pthread_mutex_t& mtx) {
		return !pthread_mutex_trylock(&mtx);
	};
	
	// Class-wrapper for pthread_mutex
	class mutex {
		pthread_mutex_t _mtx;
		
		int count = 0;
		
	public:
		
		// Allows passing mutex attributes to this instance.
		mutex(pthread_mutexattr_t& attr) {
			pthread_mutexattr_t _mtx_attr = attr;
			pthread_mutex_init(&_mtx, &_mtx_attr);
		};
		
		mutex() {
			pthread_mutexattr_t _mtx_attr;
			pthread_mutexattr_init(&_mtx_attr);
			pthread_mutexattr_settype(&_mtx_attr, PTHREAD_MUTEX_NORMAL);
			pthread_mutex_init(&_mtx, &_mtx_attr);
			pthread_mutexattr_destroy(&_mtx_attr);
		};
		
		~mutex() {
			pthread_mutex_destroy(&_mtx);
		};
		
		pthread_mutex_t& mtx() {
			return _mtx;
		};
		
		inline bool lock() {
			return !pthread_mutex_lock(&_mtx);
		};
		
		inline bool unlock() {
			return !pthread_mutex_unlock(&_mtx);
		};
		
		inline bool try_lock() {
			return !pthread_mutex_trylock(&_mtx);
		};
	};
	
	// Class-wrapper for pthread_recursive_mutex
	class recursive_mutex {
		pthread_mutex_t _mtx;
		
		int count = 0;
		
	public:
		
		recursive_mutex() {
			pthread_mutexattr_t _mtx_attr;
			pthread_mutexattr_init(&_mtx_attr);
			pthread_mutexattr_settype(&_mtx_attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&_mtx, &_mtx_attr);
			pthread_mutexattr_destroy(&_mtx_attr);
		};
		
		~recursive_mutex() {
			pthread_mutex_destroy(&_mtx);
		};
		
		pthread_mutex_t& mtx() {
			return _mtx;
		};
		
		inline bool lock() {
			return !pthread_mutex_lock(&_mtx);
		};
		
		inline bool unlock() {
			return !pthread_mutex_unlock(&_mtx);
		};
		
		inline bool try_lock() {
			return !pthread_mutex_trylock(&_mtx);
		};
	};
	
	// Equals to std::unique_lock but for pthread_mutex
	// Performs lock on being created and releases on destroy.
	// If lock fails, mutex_lock will not perform false unlock on destroy.
	class mutex_lock {
		pthread_mutex_t& _mtx;
		bool acquired;
		
	public:
	
		mutex_lock(ck_pthread::mutex& mt);
	
		mutex_lock(ck_pthread::recursive_mutex& mt);
	
		mutex_lock(pthread_mutex_t& mt);
		
		inline pthread_mutex_t& mtx() {
			return _mtx;
		};
		
		inline bool is_acquired() {
			return acquired;
		};
		
		~mutex_lock();
	};

	// Wrapper equals to the std::conditional_variable.
	// Uses pthread conditional variables.
	class cond_var {
		pthread_cond_t cv;
		pthread_condattr_t cv_attr;
		
	public:
	
		// Allows passing attributes to the cond_var
		cond_var(pthread_condattr_t& attr) {
			cv_attr = attr;
			pthread_cond_init(&cv, &cv_attr);
		};
	
		cond_var() {
			pthread_condattr_init(&cv_attr);
			pthread_cond_init(&cv, &cv_attr);
		};
		
		~cond_var() {
			pthread_cond_destroy(&cv);
			pthread_condattr_destroy(&cv_attr);
		};
		
		// Waiting on the variable.
		// On cond_var.notify() cond_fun is being executed to check wake conditions.
		void wait(ck_pthread::mutex& mtx, bool (*cond_fun) ()) {
			while (!cond_fun())
				pthread_cond_wait(&cv, &mtx.mtx());
		};
		
		void wait(ck_pthread::recursive_mutex& mtx, bool (*cond_fun) ()) {
			while (!cond_fun())
				pthread_cond_wait(&cv, &mtx.mtx());
		};
		
		// If passed mutex_lock was not qcquired condition variable will never wait
		bool wait(ck_pthread::mutex_lock& lk, bool (*cond_fun) ()) {
			if (!lk.is_acquired())
				return 0;
			
			while (!cond_fun())
				pthread_cond_wait(&cv, &lk.mtx());
			
			return 1;
		};
		
		// Sends signal to current cond_var to wake up atleast one thread and check their conditions.
		void notify() {
			pthread_cond_signal(&cv);
		};
		
		// Sends signal to current cond_var to wake up all threads and check their conditions.
		void notify_all() {
			pthread_cond_broadcast(&cv);
		};
	};

	// Wrapper for a pthread instance.
	// Used for simply creating/disposing threads.
	class thread {
		// Handler for native thread
		pthread_t _thread;
		// Handler for thread id
		pthread_attr_t _thread_attr;
		
		bool _dummy = 0;
		bool _own_attr = 0;
		
		thread() { _dummy = 1; };
		
	public:
		
		// Starts new thread by executing passed function (fun)
		//  and calling it with passes list of arguments
		thread(void* (*fun) (void*), void* args) {
			_own_attr = 1;
			pthread_attr_init(&_thread_attr);
			pthread_create(&_thread, &_thread_attr, fun, args);
		};
		
		// Starts new thread by executing passed function (fun)
		//  and calling it with passes list of arguments
		// Allows passing thread attributes to the thread.
		thread(pthread_attr_t& thread_attr, void* (*fun) (void*), void* args) {
			_thread_attr = thread_attr;
			pthread_create(&_thread, &_thread_attr, fun, args);
		};
		
		// Returns reference to the native thread
		inline pthread_t& get_thread() {
			return _thread;
		};
		
		// Returns reference to the native attributes
		inline pthread_attr_t& get_attr() {
			return _thread_attr;
		};
		
		// Returns reference to the native id
		inline uint64_t get_id() {
			uint64_t threadId = 0;
			memcpy(&threadId, &_thread, (sizeof(threadId) < sizeof(_thread) ? sizeof(threadId) : sizeof(_thread)));
			return threadId;
		};
		
		// Joins current thread, allows returning status of the execution.
		inline int join() {
			return pthread_join(_thread, 0);
		};
		
		// Returns instance of current calling thread
		static inline thread this_thread() {
			thread _this_thread;
			_this_thread._thread = pthread_self();
			
			return _this_thread;
		};
		
		// Destructor, lol
		~thread() {
			if (!_dummy) 
				if (_own_attr)
					pthread_attr_destroy(&_thread_attr);
		};
	};
};