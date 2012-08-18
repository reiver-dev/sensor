#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <zmq.h>

typedef void* MessageQueue;
typedef void* MessageQueueContext;

MessageQueueContext MessageQueue_context() {
	void *context = zmq_init(1);
	return context;
}

bool MessageQueue_contextDestroy(MessageQueueContext context) {
	assert(context);
	return zmq_term(context);
}

MessageQueue MessageQueue_getSender(MessageQueueContext context, int id) {
	assert(context);
	void *sock = zmq_socket(context, ZMQ_PUSH);
	char address[127] = {0};
	snprintf(address, 126, "inproc://#%i", id);
	int rc = zmq_connect(sock, address);
	assert(rc == 0);
	return sock;
}

MessageQueue MessageQueue_getReceiver(MessageQueueContext context, int id) {
	assert(context);
	void *sock = zmq_socket(context, ZMQ_PULL);
	char address[127] = {0};
	snprintf(address, 126, "inproc://#%i", id);
	int rc = zmq_bind(sock, address);
	assert(rc == 0);
	return sock;
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


static void zmqfree(void *buf, void *hint) {
	free(buf);
}

bool MessageQueue_send(MessageQueue queue, void *data, size_t size) {
	zmq_msg_t msg;
	bool init_success = !zmq_msg_init_data(&msg, data, size, zmqfree, NULL);
	bool send_success = !zmq_send(queue, &msg, 0);
	if (init_success)
		zmq_msg_close(&msg);
	return init_success && send_success;
}

bool MessageQueue_sendCopy(MessageQueue queue, void *data, size_t size) {
	zmq_msg_t msg;
	bool init_success =	!zmq_msg_init_size(&msg, size);
	memcpy(zmq_msg_data(&msg), data, size);
	bool send_success = !zmq_send(queue, &msg, 0);
	if (init_success) {
		zmq_msg_close(&msg);
	}
	return init_success && send_success;
}

bool MessageQueue_recv(MessageQueue queue, void **out_data, size_t *size) {
	zmq_msg_t msg;
	bool success = true;
	success = !zmq_msg_init(&msg);
	success = success && !zmq_recv(queue, &msg, 0);
	if (success) {
		*out_data = malloc(zmq_msg_size(&msg));
		memcpy(*out_data, zmq_msg_data(&msg), zmq_msg_size(&msg));
		*size = zmq_msg_size(&msg);
		zmq_msg_close(&msg);
	}
	return success;
}
