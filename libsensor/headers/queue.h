#ifndef QUEUE_H_
#define QUEUE_H_
#include <stdint.h>


typedef struct queue_item_s{
	int length;
	uint8_t* content;
	struct queue_item_s* next;
} queue_item_t;


typedef struct {
	int length;
	queue_item_t* first;
	queue_item_t* last;
} Queue_t;

//Initialize new queue
Queue_t* queue_new();
//Free memory item content and item
inline void queue_free_item(queue_item_t* self);
//Free memory of queue and all items
void queue_free(Queue_t* self);

void queue_push(Queue_t* queue, uint8_t* content, int length);
queue_item_t* queue_pop(Queue_t* self);

#endif /* QUEUE_H_ */
