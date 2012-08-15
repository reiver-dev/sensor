#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <stdbool.h>

typedef void* MessageQueue;
typedef void* MessageQueueContext;

MessageQueueContext MessageQueue_context();
void MessageQueue_contextDestroy(MessageQueueContext context);

MessageQueue MessageQueue_getSender(MessageQueueContext context, int id);
MessageQueue MessageQueue_getReceiver(MessageQueueContext context, int id);
bool MessageQueue_getPair(MessageQueueContext context, int id, MessageQueue *first, MessageQueue *second);
bool MessageQueue_destroy(MessageQueue queue);

bool MessageQueue_send(MessageQueue queue, void *data, size_t size);
bool MessageQueue_sendCopy(MessageQueue queue, void *data, size_t size);
bool MessageQueue_recv(MessageQueue queue, void **out_data, size_t *size);



#endif /* MESSAGE_QUEUE_H_ */
