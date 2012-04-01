#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "services.h"
#include "services_private.h"
#include "../debug.h"

#define MAX_SERVICES 2

static struct Service services[MAX_SERVICES];

struct Header {
	uint8_t m_version;
	uint8_t l_version;
	uint8_t header[8];
	uint8_t service;
	uint32_t length;
} __attribute__((packed));

static void makeRequest(int sock, int serviceID, uint8_t *data, int len, struct Node *to) {
	uint32_t buffer[sizeof(struct Header) + len + 1];

	struct Header header;
	header.service = serviceID;
	header.length = len;

	memcpy(buffer, &header, sizeof(header));
	memcpy(buffer + sizeof(header), data, len);

	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	if (to == NULL) {
		sockaddr.sin_addr.s_addr = INADDR_BROADCAST;
	} else {
		sockaddr.sin_addr.s_addr = to->ip4addr;
	}
	sockaddr.sin_port = 31337;

	sendto(sock, buffer, sizeof(header) + len, 0, &sockaddr, sizeof(sockaddr));
}

int extract_service(uint8_t *data, int len) {
	return 0;
}


static Service service_get(uint32_t serviceID) {
	int serv_count = sizeof(services) / sizeof(Service);
	for(int i = 0; i < serv_count; i++) {
		if (services[i].Name == serviceID) {
			return &services[i];
		}
	}

	DWARNING("Service with id not found: service=%i\n", serviceID);
	return NULL;
}

/* ---------------------- */

void Service_Request(int sock, Service service, struct Node *to, void *request) {
	struct RequestData data;

	data = service->Request(request);

	if (data.len < 0) {
		DERROR("Service request failed: service=%i\n", service->Name);
	} else {
		makeRequest(sock, service->Name, data.buffer, data.len, to);
		free(data.buffer);
		DINFO("Service request successful: service=%i\n", service->Name);
	}
}

void Services_Invoke(int sock, uint32_t serviceID, struct Node *to, void *request) {

	Service service = service_get(serviceID);
	if (service != NULL) {
		Service_Request(sock, service, to, request);
	}

	DWARNING("Service with id not found: service=%i\n", serviceID);

}

