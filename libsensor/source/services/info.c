#include <stdlib.h>

#include "services.h"
#include "info.h"

#include "../util.h"
#include "../debug.h"
#include "../nodes.h"



struct InfoItem {
	uint32_t ip4;
	uint32_t load;
};



int create_current_info(uint8_t *buffer) {
	return 0;
}



int info_request(void *request, uint8_t *buffer) {
	InfoRequest *req = request;

	int len;
	uint8_t *ptr = NULL;

	switch(req->type) {

	case INFO_TYPE_POP:
		len = 4;
		buffer = malloc(sizeof(req->type));
		ptr = buffer;
		AddToBuffer(&ptr, &req->type, sizeof(req->type));
		break;

	case INFO_TYPE_PUSH:
		len = create_current_info(buffer);
		break;

	default:
		DERROR("INFO SERVICE: %s\n", " unknown operation");
		len = -1;
		break;
	}

	return len;
}


void info_response(int sock, struct Node *to, void *request) {

	struct Node **node = nodes_get_owned();
	int nodeCount = nodes_owned_count();

	int len = nodeCount * sizeof(struct InfoItem);
	uint8_t *buffer = malloc(len);
	uint8_t *ptr = buffer;

	for (int i = 0; i < nodeCount; i++) {
		struct InfoItem *item = (struct InfoItem *)ptr;
		item->ip4 = node[i]->ip4addr;
		item->load = node[i]->info.client.load;
		ptr += sizeof(struct InfoItem);
	}

	send_service(sock, SERVICE_INFO, buffer, len, 0);
}


Service get_info_service() {
	Service service;
	service.Request = info_request;
	service.Response = info_response;
	service.Name = SERVICE_INFO;

	return service;
}

