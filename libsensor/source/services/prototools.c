#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "prototools.h"


struct Header {
	uint8_t m_version;
	uint8_t l_version;
	uint8_t header[8];
	uint8_t service;
	uint8_t request;
	uint32_t length;
} __attribute__((packed));

void send_service(int sock, int serviceID, bool request, uint8_t *data, int len, struct Node *to) {
	uint32_t buffer[4096];

	struct Header header;
	header.service = serviceID;
	header.request = request;
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
