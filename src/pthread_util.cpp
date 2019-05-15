#include "pthread_util.h"

#include <errno.h>

#include "GIL2.h"

#include "exec_util.h"

// #include <iostream>
// using namespace std;

using namespace ck_pthread;

mutex_lock::mutex_lock(ck_pthread::mutex& mt) : _mtx(mt.mtx()) {
	// wcout << "MUTEX_LOCK " << endl;
	
	int res = pthread_mutex_lock(&_mtx);
	if (res == EBUSY || res == EAGAIN)
		acquired = 0;
	else
		acquired = 1;
};

mutex_lock::mutex_lock(ck_pthread::recursive_mutex& mt) : _mtx(mt.mtx()) {
	// wcout << "RECURSIVE_MUTEX_LOCK" << endl;
	
	int res = pthread_mutex_lock(&_mtx);
	if (res == EBUSY || res == EAGAIN)
		acquired = 0;
	else
		acquired = 1;
};

mutex_lock::mutex_lock(pthread_mutex_t& mt) : _mtx(mt) {
	// wcout << "PTHREAD_MUTEX_LOCK" << endl;
	int res = pthread_mutex_lock(&_mtx);
	if (res == EBUSY || res == EAGAIN)
		acquired = 0;
	else
		acquired = 1;
};
		
mutex_lock::~mutex_lock() {
	// wcout << "MUTEX_UNLOCK" << endl;
	if (acquired)
		pthread_mutex_unlock(&_mtx);
};

	static int cnt = 0;
bool mutex::lock() {
	if (ck_core::GIL::current_thread()->get_id() == 1)
	std::wcout << "Thread " << ck_core::GIL::current_thread()->get_id() << " pre " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;
	//if (thread_cnt[ck_core::GIL::current_thread()->get_id()])
	//	throw;
	thread_cnt[ck_core::GIL::current_thread()->get_id()]++;
	if (ck_core::GIL::current_thread()->get_id() == 1)
	std::wcout << "Thread " << ck_core::GIL::current_thread()->get_id() << " " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;
	if (ck_core::GIL::current_thread()->get_id() && thread_cnt[ck_core::GIL::current_thread()->get_id() == 1]){
	print_stacktrace(128);
	std::wcout << "-----------------------------------------" << std::endl;}
	return !pthread_mutex_lock(&_mtx);
};

bool mutex::unlock() {
	thread_cnt[ck_core::GIL::current_thread()->get_id()]--;
	if (ck_core::GIL::current_thread()->get_id() == 1)
	std::wcout << "Thread " << ck_core::GIL::current_thread()->get_id() << " " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;
	return !pthread_mutex_unlock(&_mtx);
};

bool mutex::try_lock() {
	if (!pthread_mutex_trylock(&_mtx)) {
		thread_cnt[ck_core::GIL::current_thread()->get_id()]++;
	if (ck_core::GIL::current_thread()->get_id() == 1)
	std::wcout << "Thread " << ck_core::GIL::current_thread()->get_id() << " " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;
	}else
		return 0;
	return 1;
};
	
	
bool recursive_mutex::lock() {
	std::wcout << "Rec Thread " << ck_core::GIL::current_thread()->get_id() << " pre " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;
	//if (thread_cnt[ck_core::GIL::current_thread()->get_id()])
	//	throw;
	thread_cnt[ck_core::GIL::current_thread()->get_id()]++;
	std::wcout << "Rec Thread " << ck_core::GIL::current_thread()->get_id() << " " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;
	return !pthread_mutex_lock(&_mtx);
};

bool recursive_mutex::unlock() {
	thread_cnt[ck_core::GIL::current_thread()->get_id()]--;
	std::wcout << "Rec Thread " << ck_core::GIL::current_thread()->get_id() << " " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;
	return !pthread_mutex_unlock(&_mtx);
};

bool recursive_mutex::try_lock() {
	if (!pthread_mutex_trylock(&_mtx)) {
		thread_cnt[ck_core::GIL::current_thread()->get_id()]++;
	std::wcout << "Rec Thread " << ck_core::GIL::current_thread()->get_id() << " " << thread_cnt[ck_core::GIL::current_thread()->get_id()] << std::endl;}
	else
		return 0;
	return 1;
};

