#include "tsqueue.h"


tsQueue_t tsqueue_init() {
	tsQueue_t result;
	result.queue = queue_init();
	pthread_mutex_init(&result.mutex, 0);
	return result;
}

void tsqueue_destroy(tsQueue_t* self) {
	pthread_mutex_lock(&self->mutex);
	queue_destroy(&self->queue);
	pthread_mutex_unlock(&self->mutex);
}

void tsqueue_push(tsQueue_t* self, uint8_t* content, int length) {
	pthread_mutex_lock(&self->mutex);
	queue_push(&self->queue, content, length);
	pthread_mutex_unlock(&self->mutex);
}


queue_item_t* tsqueue_pop(tsQueue_t* const self) {
	queue_item_t* item;
	pthread_mutex_lock(&self->mutex);
	item = queue_pop(&self->queue);
	pthread_mutex_unlock(&self->mutex);
	return item;
}


