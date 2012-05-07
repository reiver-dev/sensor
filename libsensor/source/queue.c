#include <stdlib.h>
#include <string.h>//for memcpy
#include "queue.h"



struct queue_item_s{
	void* content;
	struct queue_item_s* next;
};


struct Queue_s{
	int length;
	queue_item_t first;
	queue_item_t last;
};


Queue_t queue_init(){
	struct Queue_s *queue;
	queue = malloc(sizeof(*queue));
	queue->length  = 0;
	queue->first   = 0;
	queue->last    = 0;
	return queue;
}


queue_item_t queue_item_new(void* content){
	queue_item_t item;
	item = malloc(sizeof(struct Queue_s));
	item->content = content;
	item->next = 0;
	return item;
}

void queue_item_destroy(queue_item_t self) {
	if(!self)
		return;
	free(self->content);
	free(self);
}


void queue_destroy(Queue_t self) {
	if (!self)
		return;

	queue_item_t item = self->first;
	while(item) {
		queue_item_t next = item->next;
		queue_item_destroy(item);
		item = next;
	}

	free(self);
}

void queue_destroy_r(Queue_t self, queue_content_destroy_f content_destroy) {
	if (!self)
		return;

	queue_item_t item = self->first;
	while(item) {
		queue_item_t next = item->next;
		content_destroy(item->content);
		queue_item_destroy(item);
		item = next;
	}
}

void queue_push(Queue_t queue, void* content) {
	queue_item_t item = queue_item_new(content);

	if (queue->last) {
		queue->last->next = item;
	}
	else{
		queue->first = item;
	}
	queue->last = item;
	queue->length++;
}

void* queue_pop(Queue_t self){
	queue_item_t item = self->first;

	if(item){
		self->first = item->next;
		self->length--;
		if (!self->length) {
			self->last = self->first;
		}
	}

	void* result = item->content;
	free(item);
	return result;
}

int queue_length(Queue_t self) {
	return self->length;
}


