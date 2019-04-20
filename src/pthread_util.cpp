#include "pthread_util.h"

#include <errno.h>
#include <iostream>
using namespace std;
using namespace ck_pthread;

mutex_lock::mutex_lock(ck_pthread::mutex& mt) : mtx(mt.mtx()) {
	int res = pthread_mutex_lock(&mtx);
	if (res == EBUSY || res == EAGAIN)
		acquired = 0;
	else
		acquired = 1;
};

mutex_lock::mutex_lock(ck_pthread::recursive_mutex& mt) : mtx(mt.mtx()) {
	int res = pthread_mutex_lock(&mtx);
	if (res == EBUSY || res == EAGAIN)
		acquired = 0;
	else
		acquired = 1;
};

mutex_lock::mutex_lock(pthread_mutex_t& mt) : mtx(mt) {
	int res = pthread_mutex_lock(&mtx);
	if (res == EBUSY || res == EAGAIN)
		acquired = 0;
	else
		acquired = 1;
};
		
mutex_lock::~mutex_lock() {
	if (acquired)
		pthread_mutex_unlock(&mtx);
};