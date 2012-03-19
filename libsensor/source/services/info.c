#include <stdlib.h>

#include "services.h"
#include "info.h"
#include "../nodes.h"
#include "prototools.h"


struct InfoItem {
	uint32_t ip4;
	uint32_t load;
};


void info_request(int sock, void *request) {
	InfoRequest *req = request;
	send_service(sock, SERVICE_INFO, true, 0, 0, req->node);
}


void info_response(int sock, uint32_t ip4_to) {

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

	send_service(sock, SERVICE_INFO, false, buffer, len, 0);
}


Service get_info_service() {
	Service service;
	service.Request = info_request;
	service.Response = info_response;
	service.Name = SERVICE_INFO;

	return service;
}

