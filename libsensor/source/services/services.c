#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "services.h"
#include "services_private.h"
#include "info.h"
#include "../debug.h"
#include "../util.h"
#include "../packet_extract.h"

#define MAX_SERVICES 2

static Service *services;
int udp_sock;

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
	sockaddr->sin_port = ServicePort;
}


static bool addressToNode(struct sockaddr_in *sockaddr, struct Node **node) {
	*node = NULL;
	if (sockaddr->sin_family != AF_INET) {
		DERROR("Sockaddr family is incorrect: %i", sockaddr->sin_family);
		return false;
	}

	if (sockaddr->sin_port == ServicePort) {
		DWARNING("Port is incorrect: %i", sockaddr->sin_port);
		return false;
	}

	uint32_t ip4addr = sockaddr->sin_addr.s_addr;
	if (ip4addr != INADDR_BROADCAST) {
		*node = node_get(ip4addr);
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
		DERROR("Header version incorrect: %i\n", header->m_version);
		result = false;
	}

	if (header->l_version == 1) {
		DERROR("Header version incorrect: %i\n", header->m_version);
		result = false;
	}

	if (strncmp((char *)header->header, "EPICIDS", 8)) {
		DERROR("%s\n", "Header string incorrect");
		result = false;
	}

	return result;
}

static bool checkHeaderLength(struct Header *header, int readLen) {
	if (header->length > readLen) {
		DERROR("Length read is insufficient: expected=%i, has=%i", header->length, readLen);
		return false;
	}
	return true;
}


static void makeRequest(int sock, int serviceID, uint8_t *data, int len, struct Node *to) {
	uint32_t buffer[sizeof(struct Header) + len + 1];

	struct Header header = initHeader();

	header.service = serviceID;
	header.length  = htonl(len + HEADER_SIZE);

	memcpy(buffer, &header, HEADER_SIZE);
	memcpy(buffer + HEADER_SIZE, data, len);

	struct sockaddr_in sockaddr;
	nodeToAddress(to, &sockaddr);

	sendto(sock, buffer, HEADER_SIZE + len, 0, &sockaddr, sizeof(sockaddr));
}

int extract_service(uint8_t *data, int len) {
	return 0;
}


static Service service_get(uint32_t serviceID) {
	int i = 0;
	while (services[i] != NULL) {
		if (services[i]->ID == serviceID) {
			return services[i];
		}
		i++;
	}

	DWARNING("Service with id not found: service=%i\n", serviceID);
	return NULL;
}


void dispatch(uint8_t *data, int len) {

}

/* ---------------------- */

void Services_Request(Service service, struct Node *to, void *request) {
	struct RequestData data;

	data = service->Request(request);

	if (data.len < 0) {
		DERROR("Service request failed: %s\n", service->Name);
	} else {
		makeRequest(udp_sock, service->ID, data.buffer, data.len, to);
		free(data.buffer);
		DINFO("Service request successful: %s\n", service->Name);
	}
}


void Services_Init(char *deviceName) {

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(ServicePort);
	bind(udp_sock, &sockaddr, sizeof(sockaddr));

	if (deviceName && *deviceName)
		setsockopt(udp_sock, SOL_SOCKET, SO_BINDTODEVICE, deviceName, strlen(deviceName));

	setNonblocking(udp_sock);

	services = malloc(sizeof(Service) * MAX_SERVICES);
	services[0] = InfoService_Get();
	services[1] = 0;
}


void Services_Destroy() {
	shutdown(udp_sock, 2);
	close(udp_sock);
	free(services);
}


void Services_Invoke(uint32_t serviceID, struct Node *to, void *request) {

	Service service = service_get(serviceID);
	if (service != NULL) {
		Services_Request(service, to, request);
	} else {
		DWARNING("Service with id not found: service=%i\n", serviceID);
	}
}

static void Services_ReceiveData(uint8_t *data, int len, struct Node *from) {
	struct Header *header = (struct Header *)data;

	if (!(checkHeaderConstans(header) && checkHeaderLength(header, len))) {
		return;
	}

	int messageLength = header->length - HEADER_SIZE;
	int serviceID = header->service;

	Service service = service_get(serviceID);

	struct RequestData requestData = {messageLength, data + HEADER_SIZE};

	if (service) {
		if (from == NULL && !service->broadcast_allowed) {
			DERROR("%s\n", "Received broadcast message on exact service");
			return;
		}

		service->Response(from, &requestData);
	}


}

void Services_Receive() {
	size_t bufferSize = 512 * sizeof(uint8_t);
	uint8_t *buffer = malloc(bufferSize);

	struct sockaddr_in address;
	socklen_t addressSize = sizeof(struct sockaddr_in);


	int bytesRead;

	time_t now = time(NULL);
	do {
		bytesRead = recvfrom(udp_sock, buffer, bufferSize, 0, &address, &addressSize);
		if (bytesRead > 0) {
			struct Node *from;
			if (addressToNode(&address, &from)) {
				Services_ReceiveData(buffer, bytesRead, from);
			} else {
				DERROR("%s\n", "incorrect receive address");
			}
		}

	} while (bytesRead > 0 && time(NULL) - now < 2);

	if (bytesRead == -1) {
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

	struct Node *from = node_get(ip_header->saddr);
	struct Node *to = node_get(ip_header->daddr);

	if ((from && to)
		&& (from != to)
		&& from->type == NODE_TYPE_SENSOR
		&& node_is_me(to))
	{

		uint8_t *data = packet_map_udp_payload(buffer, len);
		if (data
			&& checkHeaderConstans((struct Header *)data)
			&& checkHeaderLength((struct Header *)data, len - ((uintptr_t)data - (uintptr_t)buffer)))
		{
			return true;
		}

	}

	return false;

}
