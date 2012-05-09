#include <stdlib.h>
#include <assert.h>

#include "services.h"
#include "services_private.h"
#include "node.h"

#include "../array.h"
#include "../util.h"
#include "../debug.h"
#include "../nodes.h"

#define NODEID 1
#define ITEM_SIZE sizeof(uint32_t)

struct RequestData node_request(ServicesData servicesData, void *request);
void node_response(ServicesData servicesData, struct Node *from, struct RequestData *request);


static struct Service nodeService = {
	.Request  = node_request,
	.Response = node_response,
	.ID       = NODEID,
	.Name     = "Node Service",
	.broadcast_allowed = false
};


static struct RequestData send_nodes(uint8_t type, Array ip4array) {
	assert(ip4array);
	assert(Array_checkType(ip4array, sizeof(uint32_t)));

	uint8_t *buffer, *ptr;
	int len = Array_length(ip4array) * (ITEM_SIZE) + 5;

	buffer = malloc(len);
	ptr = buffer;

	AddToBuffer8(&ptr, type);
	AddToBuffer32(&ptr, len);
	for (int i = 0; i < len; i++) {
		AddToBuffer32NoOrder(&ptr, ARRAY_GET(ip4array, uint32_t, i));
	}

	struct RequestData result = {len, buffer};
	return result;
}

struct RequestData node_request(ServicesData servicesData, void *request) {
	NodeRequest *req = request;

	struct RequestData data;

	switch(req->type) {

	case NODESERVICE_TYPE_GIVE:
		data = send_nodes(req->type, req->ip4array);
		break;

	case NODESERVICE_TYPE_TAKE:
		data = send_nodes(req->type, req->ip4array);
		break;

	default:
		DERROR("INFO SERVICE: %s\n", " unknown operation");
		data.len = -1;
		data.buffer = NULL;
		break;
	}

	return data;
}


void node_response(ServicesData servicesData, struct Node *from, struct RequestData *request) {
	uint8_t *ptr = request->buffer;

	int type  = GetFromBuffer8(&ptr);

	if (type == NODESERVICE_TYPE_TAKE) {

		size_t len = GetFromBuffer32(&ptr);
		if (request->len - 1 < len * ITEM_SIZE) {
			DWARNING("NODE SERVICE: node length is insufficient = %i\n", request->len - 1);
			return;
		}


		Array array = Array_init(0, sizeof(uint32_t));
		for (size_t i = 0; i < len; i++) {
			uint32_t ip4addr = GetFromBuffer32NoOrder(&ptr);
			struct Node *client = node_get(ip4addr);
			if (client != NULL && client->type == NODE_TYPE_CLIENT && node_is_me(client->info.client.owned_by)) {
				Array_add(array, &ip4addr);
			}
		}

		if (Array_length(array) > 0) {
			NodeRequest request = {
					.type = NODESERVICE_TYPE_GIVE,
					.ip4array = array
			};
			Services_Request(servicesData, NodeService_Get(), from, &request);
		}
		Array_destroy(array);

	} else if (type == NODESERVICE_TYPE_GIVE) {

		size_t len = GetFromBuffer32(&ptr);
		if (request->len - 1 < len * ITEM_SIZE) {
			DWARNING("NODE SERVICE: node length is insufficient = %i\n", request->len - 1);
			return;
		}

		int ip4addr;
		for (int i = 0; i < len; i++) {
			ip4addr = GetFromBuffer32NoOrder(&ptr);
			node_take(node_get(ip4addr));
		}
	}

}


Service NodeService_Get() {
	return &nodeService;
}
