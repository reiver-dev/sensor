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
	struct Node **nodes = nodes_get_owned();
	int nodeCount = nodes_owned_count();

	int len = nodeCount * (sizeof(struct InfoItem)) + 4;
	buffer = malloc(len);

	AddToBuffer8(&buffer, INFO_TYPE_PUSH);
	AddToBuffer32(&buffer, nodeCount);
	for (int i = 0; i < nodeCount; i++) {
		AddToBuffer32(&buffer, nodes[i]->ip4addr);
		AddToBuffer32(&buffer, nodes[i]->info.client.load);
	}

	return len;
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
		AddToBuffer8(&ptr, req->type);
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
	uint8_t *buffer = request;
	int type  = GetFromBuffer8(&buffer);
	int count = GetFromBuffer32(&buffer);
}


Service get_info_service() {
	Service service;
	service.Request  = info_request;
	service.Response = info_response;
	service.Name     = SERVICE_INFO;

	return service;
}

