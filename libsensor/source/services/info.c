#include <stdlib.h>

#include "services.h"
#include "services_private.h"
#include "info.h"

#include "../balancing.h"
#include "../util.h"
#include "../debug.h"
#include "../nodes.h"

#define INFOID 0

#define ITEM_SIZE (sizeof(uint32_t) * 2 + ETH_ALEN)

/* prototypes */
static struct RequestData info_request(ServicesData servicesData, void *request);
static void info_response(ServicesData servicesData, struct Node *from, struct RequestData *data);


static struct Service infoService = {
	.Request  = info_request,
	.Response = info_response,
	.ID       = INFOID,
	.Name     = "Info Service",
	.broadcast_allowed = true
};

static struct RequestData pop_info() {
	uint8_t *buffer, *ptr;
	size_t len = 1;

	buffer = malloc(len);
	ptr = buffer;

	AddToBuffer8(&ptr, INFO_TYPE_POP);

	struct RequestData result = {len, buffer};
	return result;
}

static struct RequestData push_info(ServicesData servicesData) {
	uint8_t *buffer, *ptr;

	ArrayList ownedNodes = balancing_get_owned(servicesData->balancer);
	int ownedCount = ArrayList_length(ownedNodes);

	size_t len = ownedCount * (ITEM_SIZE) + 5;

	buffer = malloc(len);
	ptr = buffer;

	AddToBuffer8(&ptr, INFO_TYPE_PUSH);
	AddToBuffer32(&ptr, ownedCount);
	for (int i = 0; i < ownedCount; i++) {
		struct Node *node = ARRAYLIST_GET(ownedNodes, struct Node*, i);
		if (node->current_load != NODE_LOAD_NOT_READY)
			node->load = node->current_load;
		AddToBuffer32NoOrder(&ptr, node->ip4addr);
		AddToBuffer32(&ptr, node->load);
		AddToBuffer(&ptr, node->hwaddr, ETH_ALEN);
	}

	struct RequestData result = {len, buffer};
	return result;
}

static struct RequestData info_request(ServicesData servicesData, void *request) {
	InfoRequest *req = request;

	struct RequestData data;

	switch(req->type) {

	case INFO_TYPE_POP:
		data = pop_info(servicesData);
		break;

	case INFO_TYPE_PUSH:
		data = push_info(servicesData);
		break;

	default:
		DERROR("INFO SERVICE: %s\n", " unknown operation");
		data.len = -1;
		data.buffer = NULL;
		break;
	}

	return data;
}


static void info_response(ServicesData servicesData, struct Node *from, struct RequestData *data) {
	uint8_t *ptr = data->buffer;

	int type  = GetFromBuffer8(&ptr);

	if (type == INFO_TYPE_POP) {

		InfoRequest request = {INFO_TYPE_PUSH};
		Services_Request(servicesData, InfoService_Get(), 0, &request);

	} else if (type == INFO_TYPE_PUSH) {

		int count = GetFromBuffer32(&ptr);

		if (data->len - 1 < count * ITEM_SIZE) {
			DWARNING("INFO SERVICE: info length is insufficient = %i\n", data->len - 1);
			return;
		}

		uint32_t ip4addr;
		uint32_t load;
		uint8_t hw[ETH_ALEN];
		for (int i = 0; i < count; i++) {
			ip4addr = GetFromBuffer32NoOrder(&ptr);
			load = GetFromBuffer32(&ptr);
			GetFromBuffer(&ptr, hw, ETH_ALEN);

			node_answered(ip4addr, hw);
			balancing_node_owned(servicesData->balancer, from->ip4addr, ip4addr, load);
		}

		balancing_sensor_info_refreshed(servicesData->balancer, from->ip4addr);
	}
}


Service InfoService_Get() {
	return &infoService;
}

