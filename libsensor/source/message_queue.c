#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <zmq.h>

#include "message_queue.h"
#include "debug.h"

MessageQueueContext MessageQueue_context() {
	void *context = zmq_init(1);
	return context;
}

bool MessageQueue_contextDestroy(MessageQueueContext context) {
	assert(context);
	return zmq_term(context);
}

bool MessageQueue_getPair(MessageQueueContext context, int id, MessageQueue *first, MessageQueue *second) {
	assert(context);
	int rc;
	char address[127] = {0};
	snprintf(address, 126, "inproc://#%i", id);

	*first = zmq_socket(context, ZMQ_PAIR);
	rc = zmq_bind(*first, address);
	assert(rc == 0);

	*second = zmq_socket(context, ZMQ_PAIR);
	rc = zmq_connect(*second, address);
	assert(rc == 0);

	return true;
}

bool MessageQueue_destroy(MessageQueue queue) {
	assert(queue);
	return zmq_close(queue);
}

bool MessageQueue_send(MessageQueue queue, int type, void *data, size_t data_size) {

	zmq_msg_t msg;

	bool init_success =	!zmq_msg_init_size(&msg, sizeof(type) + data_size);

	char *msg_data = zmq_msg_data(&msg);

	memcpy(msg_data, &type, sizeof(type));

	if (data && data_size) {
		msg_data += sizeof(type);
		memcpy(msg_data, data, data_size);
	}

	bool send_success = !zmq_send(queue, &msg, 0);
	if (init_success) {
		zmq_msg_close(&msg);
	}

	return init_success && send_success;
}

bool MessageQueue_recv(MessageQueue queue, int *type, void *data, size_t *data_size) {
	zmq_msg_t msg;
	bool success = true;
	success = !zmq_msg_init(&msg);
	success = success && !zmq_recv(queue, &msg, 0);
	if (success) {

		size_t of_type = sizeof(*type);
		size_t of_data = zmq_msg_size(&msg) - of_type;

		char *msg_data = zmq_msg_data(&msg);
		memcpy(type, msg_data, of_type);

		if (of_data) {
			msg_data += of_type;
			memcpy(data, msg_data, of_data);
		}

		*data_size = of_data;

		zmq_msg_close(&msg);
	}
	return success;
}
