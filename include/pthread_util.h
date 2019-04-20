#pragma once

#include <pthread.h>

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
		
	public:
		
		mutex() {
			pthread_mutex_init(&_mtx, 0);
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
		pthread_mutexattr_t mtx_attr;
		
	public:
		
		recursive_mutex() {
			pthread_mutexattr_init(&mtx_attr);
			pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&_mtx, &mtx_attr);
		};
		
		~recursive_mutex() {
			pthread_mutex_destroy(&_mtx);
		};
		
		pthread_mutex_t& mtx() {
			return _mtx;
		};
		
		pthread_mutex_t& mutex() {
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
	class mutex_lock {
		pthread_mutex_t& mtx;
		bool acquired;
		
	public:
	
		mutex_lock(ck_pthread::mutex& mt);
	
		mutex_lock(ck_pthread::recursive_mutex& mt);
	
		mutex_lock(pthread_mutex_t& mt);
		
		~mutex_lock();
	};
};