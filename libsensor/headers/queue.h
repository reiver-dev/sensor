#ifndef QUEUE_H_
#define QUEUE_H_
#include <stdint.h>


typedef struct queue_item_s{
	void* content;
	struct queue_item_s* next;
} queue_item_t;


typedef struct {
	int length;
	queue_item_t* first;
	queue_item_t* last;
} Queue_t;

//Initialize new queue
Queue_t queue_init();
//Free memory item content and item
void queue_item_destroy(queue_item_t* self);
//Free memory of all items
void queue_destroy(Queue_t* self);
void queue_push(Queue_t* queue, void* item);
void* queue_pop(Queue_t* self);

#endif /* QUEUE_H_ */
