#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <stdbool.h>

typedef void* MessageQueue;
typedef void* MessageQueueContext;

MessageQueueContext MessageQueue_context();
bool MessageQueue_contextDestroy(MessageQueueContext context);

bool MessageQueue_getPair(MessageQueueContext context, int id, MessageQueue *first, MessageQueue *second);
bool MessageQueue_destroy(MessageQueue queue);

bool MessageQueue_send(MessageQueue queue, int type, void *data, size_t data_size);
bool MessageQueue_recv(MessageQueue queue, int *type, void *data, size_t *data_size);



#endif /* MESSAGE_QUEUE_H_ */
