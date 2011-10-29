#ifndef TSQUEUE_H_
#define TSQUEUE_H_
#include <pthread.h>
#include "queue.h"


typedef struct tsQueue_s {
	Queue_t queue;
	pthread_mutex_t mutex;
} tsQueue_t;




#endif /* TSQUEUE_H_ */
