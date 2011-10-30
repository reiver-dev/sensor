#include <stdlib.h>
#include <string.h>//for memcpy
#include "queue.h"


Queue_t queue_init(){
	Queue_t queue;
	queue.length = 0;
	queue.first = 0;
	queue.last = 0;
	return queue;
}

void queue_item_destroy(queue_item_t* self) {
	free(self->content);
	free(self);
}

void queue_destroy(Queue_t* self){
	queue_item_t* item = self->first;
	while(item) {
		queue_item_t* next = item->next;
		queue_item_destroy(item);
		item = next;
	}
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

void queue_push_copy(Queue_t* queue, uint8_t* content, int length){
	uint8_t* newcontent = malloc(length);
	memcpy(newcontent, content, length);
	queue_push(queue, newcontent, length);
}

queue_item_t* queue_pop(Queue_t* self){
	queue_item_t* item = self->first;
	if(item){
		self->first = item->next;
		self->length--;
	}
	return item;
}



