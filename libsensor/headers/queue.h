#ifndef QUEUE_H_
#define QUEUE_H_
#include <stdint.h>


typedef struct queue_item_s *queue_item_t;
typedef struct Queue_s *Queue_t;

typedef void (*queue_content_destroy_f)(void*);

//Initialize new queue
Queue_t queue_init();


//Free memory item content and item
void queue_item_destroy(queue_item_t self);
//Free memory of all items
void queue_destroy(Queue_t self);
void queue_destroy_r(Queue_t self, queue_content_destroy_f content_destroy);



void queue_push(Queue_t queue, void* item);
void* queue_pop(Queue_t self);
int queue_length(Queue_t self);

#endif /* QUEUE_H_ */
