#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "services.h"
#include "services_private.h"
#include "info.h"
#include "node.h"

#include "../debug.h"
#include "../util.h"
#include "../packet_extract.h"

#define MAX_SERVICES 2


struct Services {
	int udp_sock;
	ArrayList services;
};

static const uint16_t ServicePort = 31337;

struct Header {
	uint8_t m_version;
	uint8_t l_version;
	uint8_t header[8];
	uint8_t service;
	uint32_t length;
} __attribute__((packed));
#define HEADER_SIZE sizeof(struct Header)


static void nodeToAddress(struct Node *node, struct sockaddr_in *sockaddr) {
	sockaddr->sin_family = AF_INET;
	if (node == NULL) {
		sockaddr->sin_addr.s_addr = INADDR_BROADCAST;
	} else {
		sockaddr->sin_addr.s_addr = node->ip4addr;
	}
	sockaddr->sin_port = htons(ServicePort);
}


static bool addressToNode(struct sockaddr_in *sockaddr, struct Node **node) {
	*node = NULL;
	if (sockaddr->sin_family != AF_INET) {
		DERROR("Sockaddr family is incorrect (%i)\n", sockaddr->sin_family);
		return false;
	}

	uint32_t ip4addr = sockaddr->sin_addr.s_addr;
	if (ip4addr != INADDR_BROADCAST) {
		*node = node_get(ip4addr);
	}

	if (node_is_me(*node)) {
		DWARNING("Got mirror request from (%s:%i)\n", Ip4ToStr(sockaddr->sin_addr.s_addr), ntohs(sockaddr->sin_port));
		return false;
	}

	return true;
}

static struct Header initHeader() {
	struct Header header = {
		.m_version = 0,
		.l_version = 1,
		.header = {'E', 'P', 'I', 'C', 'I', 'D', 'S', '!'},
		.service = 0,
		.length = 0
	};
	return header;
}

static bool checkHeaderConstans(struct Header *header) {
	bool result = true;

	if (header->m_version != 0) {
		DERROR("Header major version incorrect: %i\n", header->m_version);
		result = false;
	}

	if (header->l_version != 1) {
		DERROR("Header lower version incorrect: %i\n", header->l_version);
		result = false;
	}

	if (strncmp((char *)header->header, "EPICIDS!", 8)) {
		DERROR("%s\n", "Header string incorrect");
		result = false;
	}

	return result;
}

static bool checkHeaderLength(struct Header *header, int readLen) {
	if (ntohl(header->length) > readLen) {
		DERROR("Length read is insufficient: expected=%i, has=%i", header->length, readLen);
		return false;
	}
	return true;
}


static int makeRequest(int sock, int serviceID, uint8_t *data, int len, struct Node *to) {
	uint8_t buffer[HEADER_SIZE + len];
	memset(buffer, 0, HEADER_SIZE + len);

	struct Header header = initHeader();

	header.service = serviceID;
	header.length  = htonl(len + HEADER_SIZE);

	memcpy(buffer, &header, HEADER_SIZE);
	memcpy(buffer + HEADER_SIZE, data, len);

	struct sockaddr_in sockaddr;
	nodeToAddress(to, &sockaddr);

	return sendto(sock, buffer, HEADER_SIZE + len, 0, &sockaddr, sizeof(sockaddr));

}

int extract_service(uint8_t *data, int len) {
	return 0;
}



static bool find_by_serviceID(void *service, void *serviceID) {
	return ((Service)service)->ID == *(uint32_t *)serviceID;
}

static Service service_get(ArrayList services, uint32_t serviceID) {
	Service service = ArrayList_find(services, &serviceID, find_by_serviceID);
	if (!service)
		DWARNING("Service with id not found: service=%i\n", serviceID);
	return service;
}


void dispatch(uint8_t *data, int len) {

}

/* ---------------------- */

void Services_Request(ServicesData self, Service service, struct Node *to, void *request) {
	struct RequestData data;

	data = service->Request(self, request);

	if (data.len <= 0 || data.buffer == NULL) {
		DERROR("Service request failed: %s (Protocol error)\n", service->Name);
	} else {
		int res = makeRequest(self->udp_sock, service->ID, data.buffer, data.len, to);
		free(data.buffer);
		if (res != -1) {
			DINFO("Service request successful: %s\n", service->Name);
		} else {
			DINFO("Service request failed: %s (%s)\n", service->Name, strerror(errno));
		}
	}
}


ServicesData Services_Init(char *deviceName) {

	int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(ServicePort);
	bind(udp_sock, &sockaddr, sizeof(sockaddr));

	if (deviceName && *deviceName) {
		if (setsockopt(udp_sock, SOL_SOCKET, SO_BINDTODEVICE, deviceName, strlen(deviceName)) == -1)
			DERROR("Service socket SO_BINDTODEVICE to %s failed (%s)\n", deviceName, strerror(errno));
	}
	int val = 1;
	if(setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) == -1)
		DERROR("Service socket SO_BROADCAST failed (%s)\n", strerror(errno));

	setNonblocking(udp_sock);

	ArrayList services = ArrayList_init(3, 0);
	ArrayList_add(services, InfoService_Get());
	ArrayList_add(services, NodeService_Get());

	struct Services *self = malloc(sizeof(*self));
	self->udp_sock = udp_sock;
	self->services = services;

	return self;

}


void Services_Destroy(ServicesData self) {
	shutdown(self->udp_sock, 2);
	close(self->udp_sock);
	ArrayList_destroy(self->services);
	free(self);
}


void Services_Invoke(ServicesData self, uint32_t serviceID, struct Node *to, void *request) {

	Service service = service_get(self->services, serviceID);
	if (service != NULL) {
		Services_Request(self, service, to, request);
	} else {
		DWARNING("Service with id not found: service=%i\n", serviceID);
	}
}

static void Services_ReceiveData(ServicesData self, uint8_t *data, int len, struct Node *from) {
	struct Header *header = (struct Header *)data;

	if (!(checkHeaderConstans(header) && checkHeaderLength(header, len))) {
		return;
	}

	int messageLength = ntohl(header->length) - HEADER_SIZE;
	int serviceID = header->service;

	Service service = service_get(self->services, serviceID);

	struct RequestData requestData = {messageLength, data + HEADER_SIZE};

	if (service) {
		if (from == NULL && !service->broadcast_allowed) {
			DERROR("%s\n", "Received broadcast message on exact service");
			return;
		}

		service->Response(self, from, &requestData);
	}


}

void Services_Receive(ServicesData self) {
	size_t bufferSize = 512 * sizeof(uint8_t);
	uint8_t *buffer = malloc(bufferSize);

	struct sockaddr_in address;
	socklen_t addressSize = sizeof(struct sockaddr_in);


	int bytesRead;

	time_t now = time(NULL);
	do {
		bytesRead = recvfrom(self->udp_sock, buffer, bufferSize, 0, &address, &addressSize);
		if (bytesRead > 0) {
			DINFO("Service received: %i bytes\n", bytesRead);
			struct Node *from;
			if (addressToNode(&address, &from)) {
				Services_ReceiveData(self, buffer, bytesRead, from);
			} else {
				DERROR("%s\n", "incorrect receive address");
			}
		}

	} while (bytesRead > 0 && time(NULL) - now < 2);

	if (bytesRead == -1 && errno != EWOULDBLOCK) {
		DERROR("%s\n", "reading socket failed");
	}

	free(buffer);
}

bool Services_isResponse(uint8_t *buffer, int len) {
	struct iphdr *ip_header = packet_map_ip(buffer, len);
	struct udphdr *udp_header = packet_map_udp(buffer, len);
	if (!(ip_header && udp_header)) {
		return false;
	}

	struct Node *from = node_get_destination(ip_header->saddr);
	struct Node *to = node_get_destination(ip_header->daddr);

	if (from == NULL || from == to) /* if sender is unknown record it*/
		return false;

	/* if from any sensor to me OR broadcast */
	if (from->type == NODE_TYPE_SENSOR && (to == NULL || node_is_me(to))) {
		uint8_t *data = packet_map_payload(buffer, len);
		if (data
			&& checkHeaderConstans((struct Header *)data)
			&& checkHeaderLength((struct Header *)data, len - ((uintptr_t)data - (uintptr_t)buffer)))
		{
			return true;
		}
	}

	return false;

}
