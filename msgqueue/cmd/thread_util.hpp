/*
 * thread_util.hpp
 *
 *  Created on: 04.12.2012
 *      Author: reiver
 */

#ifndef THREAD_UTIL_HPP_
#define THREAD_UTIL_HPP_

#include <pthread.h>

namespace mq {

class Mutex {
public:
	Mutex() {
		pthread_mutex_init(&m, 0);
	}

	~Mutex() {
		pthread_mutex_destroy(&m);
	}

	void lock() {
		pthread_mutex_lock(&m);
	}

	void unlock() {
		pthread_mutex_unlock(&m);
	}

	friend class CondVar;

private:

	pthread_mutex_t *get() {
		return &m;
	}

	pthread_mutex_t m;
};


class CondVar {
public:
	CondVar() {
		pthread_cond_init(&cv, 0);
	}

	~CondVar() {
		pthread_cond_destroy(&cv);
	}

	void wait(Mutex &m) {
		pthread_cond_wait(&cv, m.get());
	}

	void wait_for(Mutex &m, int sec, int nanosec) {
		timespec time = {sec, nanosec};
		pthread_cond_timedwait(&cv, m.get(), &time);
	}

	void notify() {
		pthread_cond_signal(&cv);
	}

	void notify_all() {
		pthread_cond_broadcast(&cv);
	}

private:

	pthread_cond_t *get() {
		return &cv;
	}

	pthread_cond_t cv;
};

}

#endif /* THREAD_UTIL_HPP_ */
