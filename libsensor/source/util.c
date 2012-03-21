#include <string.h>

#include <arpa/inet.h>
#include <netinet/ether.h>

#include <sys/ioctl.h>             // For interface flags change

#include <netinet/in.h>            // Basic address related functions and types
#include <netinet/ether.h>
#include <net/if.h>

#include <netpacket/packet.h>

#include "util.h"
#include "debug.h"

char *Ip4ToStr(const uint32_t ip) {
	return inet_ntoa(*(struct in_addr *)&ip);
}

char *EtherToStr(const uint8_t eth[ETH_ALEN]) {
	return ether_ntoa((struct ether_addr*)eth);
}


int bitcount(unsigned int n) {
	unsigned int count;
	count = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
	return ((count + (count >> 3)) & 030707070707) % 63;
}



int bind_socket_to_interface(int sock, char *interfaceName) {

	struct ifreq interface;
	struct sockaddr_ll address;

	strcpy(interface.ifr_name, interfaceName);

	// Get interface index
	if (ioctl(sock, SIOCGIFINDEX, &interface) == -1) {
		DERROR("%s\n", "get interface flags failed");
		return 1;
	}

	// Bind raw socket to interface
	address.sll_family   = AF_PACKET;
	address.sll_ifindex  = interface.ifr_ifindex;
	address.sll_protocol = htons(ETH_P_ALL);

	if (bind(sock, (struct sockaddr *)&address, sizeof(address)) == -1) {
		DERROR("%s\n", "bind socket to interface failed");
		return 1;
	}

	return 0;
}

void AddToBuffer(uint8_t **bufpointer, void *data, size_t ofData) {
	memcpy(*bufpointer, data, ofData);
	*bufpointer += ofData;
}


void AddToBuffer32(uint8_t **bufpointer, uint32_t data) {
	uint32_t toWrite = htonl(data);
	memcpy(*bufpointer, &toWrite, 4);
	*bufpointer += 4;
}

void AddToBuffer16(uint8_t **bufpointer, uint16_t data) {
	uint16_t toWrite = htons(data);
	memcpy(*bufpointer, &toWrite, 2);
	*bufpointer += 2;
}

void AddToBuffer8(uint8_t **bufpointer, uint8_t data) {
	**bufpointer = data;
	(*bufpointer)++;
}

void AddToBuffer4(uint8_t **bufpointer, uint8_t data8, uint8_t data4) {
	uint8_t toWrite = (data8 << 4) & (data4 | 0xf0);
	**bufpointer = toWrite;
	(*bufpointer)++;
}

void AddToBuffer1(uint8_t *bufpointer, uint8_t data, int place) {
	uint8_t toWrite = (data & 0xFE) << place;
	*bufpointer = toWrite;
}

uint32_t GetFromBuffer32(uint8_t **bufpointer) {
	uint8_t get[4];
	memcpy(get, *bufpointer, 4);
	uint32_t *data = (uint32_t *)get;
	bufpointer += 4;
	return ntohl(*data);
}

uint16_t GetFromBuffer16(uint8_t **bufpointer) {
	uint8_t get[2];
	memcpy(get, *bufpointer, 2);
	uint16_t *data = (uint16_t *)get;
	bufpointer += 2;
	return ntohs(*data);
}

uint8_t GetFromBuffer8(uint8_t **bufpointer) {
	uint8_t data = **bufpointer;
	bufpointer++;
	return data;
}

void GetFromBuffer4(uint8_t **bufpointer, uint8_t *data8, uint8_t *data4) {
	if (data8 != NULL) {
		*data8 = **bufpointer >> 4;
	}
	if (data4 != NULL) {
		*data4 = **bufpointer & 0xF0;
	}
	bufpointer++;
}

uint8_t GetFromBuffer1(const uint8_t *bufpointer, int place) {
	return (*bufpointer >> place) & 0xFE;
}
