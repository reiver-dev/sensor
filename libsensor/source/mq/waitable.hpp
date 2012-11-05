#ifndef WAITABLE_HPP_
#define WAITABLE_HPP_

#include <pthread.h>

class Waitable {

	pthread_mutex_t mtx;
	pthread_cond_t cond;

public:

	Waitable() {
		pthread_mutex_init(&mtx, 0);
		pthread_cond_init(&cond, 0);
	}

	~Waitable() {
		pthread_mutex_destroy(&mtx);
		pthread_cond_destroy(&cond);
	}

	void wait() {
		pthread_mutex_lock(&mtx);
		pthread_cond_wait(&cond, &mtx);
	}

	void notify() {
		pthread_cond_signal(&cond);
	}

};


#endif /* WAITABLE_HPP_ */
