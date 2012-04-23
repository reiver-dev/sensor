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
static void info_response(struct Node *from, struct RequestData *data);


static struct Service infoService = {
	.Request  = info_request,
	.Response = info_response,
	.ID       = SERVICE_INFO,
	.Name     = "Info Service"
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

	ArrayList ownedNodes = nodes_get_owned();
	int ownedCount = ArrayList_length(ownedNodes);

	int len = ownedCount * (ITEM_SIZE) + 5;

	buffer = malloc(len);
	ptr = buffer;

	AddToBuffer8(&ptr, INFO_TYPE_PUSH);
	AddToBuffer32(&ptr, ownedCount);
	for (int i = 0; i < ownedCount; i++) {
		AddToBuffer32NoOrder(&ptr, ARRAYLIST_GET(ownedNodes, struct Node*, i)->ip4addr);
		AddToBuffer32(&ptr, ARRAYLIST_GET(ownedNodes, struct Node*, i)->info.client.load.timestamp);
		AddToBuffer32(&ptr, ARRAYLIST_GET(ownedNodes, struct Node*, i)->info.client.load.load);
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


static void info_response(struct Node *from, struct RequestData *data) {
	uint8_t *ptr = data->buffer;

	int type  = GetFromBuffer8(&ptr);

	if (type == INFO_TYPE_POP) {

		InfoRequest request;
		request.type = INFO_TYPE_PUSH;
		Services_Request(InfoService_Get(), 0, &request);

	} else if (type == INFO_TYPE_PUSH) {

		int count = GetFromBuffer32(&ptr);

		if (data->len - 1 < count * ITEM_SIZE) {
			DWARNING("INFO SERVICE: info length is insufficient = %i", data->len - 1);
			return;
		}

		int ip4addr;
		struct NodeLoad load;
		for (int i = 0; i < count; i++) {
			ip4addr = GetFromBuffer32NoOrder(&ptr);
			load.timestamp = GetFromBuffer32(&ptr);
			load.load = GetFromBuffer32(&ptr);

			node_set_owned_by(from, ip4addr, load);
		}
	}
}


Service InfoService_Get() {
	return &infoService;
}

