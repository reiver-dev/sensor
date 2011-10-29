#include <stdlib.h>
#include "queue.h"


Queue_t* queue_new(){
	Queue_t* queue = (Queue_t*)malloc(sizeof(Queue_t));
	queue->length = 0;
	queue->first = 0;
	queue->last = 0;
	return queue;
}

inline void queue_free_item(queue_item_t* self){
	free(self->content);
	free(self);
}

void queue_free(Queue_t* self){
	queue_item_t* item = self->first;
	while(item) {
		queue_item_t* next = item->next;
		queue_free_item(item);
		item = next;
	}
	free(self);
}


queue_item_t* queue_new_item(uint8_t* content, int length){

	queue_item_t* newitem = malloc(sizeof(queue_item_t));
	newitem->content = content;
	newitem->length = length;
	newitem->next = 0;

	return newitem;
}

void queue_push(Queue_t* queue, uint8_t* content, int length){
	queue_item_t* item = queue_new_item(content, length);

	if (queue->last) {
		queue->last->next = item;
	}
	else{
		queue->first = item;
	}
	queue->last = item;

	queue->length++;
}

queue_item_t* queue_pop(Queue_t* self){
	queue_item_t* item = self->first;
	if(item){
		self->first = item->next;
		self->length--;
	}
	return item;
}



