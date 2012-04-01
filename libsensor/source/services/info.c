#include <stdlib.h>

#include "services.h"
#include "services_private.h"
#include "info.h"

#include "../util.h"
#include "../debug.h"
#include "../nodes.h"


#define ITEM_SIZE sizeof(uint32_t) * 2

/* prototypes */
static struct RequestData info_request(void *request);
static void info_response(int sock, struct Node *from, struct RequestData data);


static struct Service infoService = {
	.Request  = info_request,
	.Response = info_response,
	.Name     = SERVICE_INFO
};

static struct RequestData put_type(int type) {
	uint8_t *buffer, *ptr;
	int len = 4;

	buffer = malloc(len);
	ptr = buffer;

	AddToBuffer8(&ptr, type);

	struct RequestData result = {len, buffer};
	return result;
}

static struct RequestData put_info() {
	uint8_t *buffer, *ptr;

	struct Node **nodes = nodes_get_owned();
	int nodeCount = nodes_owned_count();

	int len = nodeCount * (ITEM_SIZE) + 4;

	buffer = malloc(len);
	ptr = buffer;

	AddToBuffer8(&ptr, INFO_TYPE_PUSH);
	AddToBuffer32(&ptr, nodeCount);
	for (int i = 0; i < nodeCount; i++) {
		AddToBuffer32(&ptr, nodes[i]->ip4addr);
		AddToBuffer32(&ptr, nodes[i]->info.client.load);
	}

	struct RequestData result = {len, buffer};
	return result;
}

static struct RequestData info_request(void *request) {
	InfoRequest *req = request;

	struct RequestData data;

	switch(req->type) {

	case INFO_TYPE_POP:
		data = put_type(req->type);
		break;

	case INFO_TYPE_PUSH:
		data = put_info();
		break;

	default:
		DERROR("INFO SERVICE: %s\n", " unknown operation");
		data.len = -1;
		data.buffer = NULL;
		break;
	}

	return data;
}


static void info_response(int sock, struct Node *from, struct RequestData data) {
	uint8_t *ptr = data.buffer;

	int type  = GetFromBuffer8(&ptr);

	if (type == INFO_TYPE_POP) {

		InfoRequest request;
		request.type = INFO_TYPE_PUSH;
		Service_Request(sock, InfoService_Get(), 0, &request);

	} else if (type == INFO_TYPE_PUSH) {

		int count = GetFromBuffer32(&ptr);

		if (data.len - 1 < count * ITEM_SIZE) {
			DWARNING("INFO SERVICE: info length is insufficient = %i", data.len - 1);
			return;
		}

		int ip4addr;
		int load;
		for (int i = 0; i < count; i++) {
			ip4addr = GetFromBuffer32(&ptr);
			load = GetFromBuffer32(&ptr);
			node_set_owned_by(from, ip4addr, load);
		}
	}
}


Service InfoService_Get() {
	return &infoService;
}

