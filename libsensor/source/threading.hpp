#ifndef THREADING_HPP_
#define THREADING_HPP_

#include <pthread.h>

class Lock {
private:
	pthread_mutex_t lock;

public:
	Lock() {
		pthread_mutex_init(&lock, NULL);
	}
	~Lock() {
		pthread_mutex_destroy(&lock);
	}
	void aquire() {
		pthread_mutex_lock(&lock);
	}
	void release() {
		pthread_mutex_unlock(&lock);
	}
};

class RwLock {
private:
	pthread_rwlock_t lock;

public:
	RwLock() {
		pthread_rwlock_init(&lock, NULL);
	}
	~RwLock() {
		pthread_rwlock_destroy(&lock);
	}
	void lockRead() {
		pthread_rwlock_rdlock(&lock);
	}
	void lockWrite() {
		pthread_rwlock_wrlock(&lock);
	}
	void release() {
		pthread_rwlock_unlock(&lock);
	}
};


#endif /* THREADING_HPP_ */
