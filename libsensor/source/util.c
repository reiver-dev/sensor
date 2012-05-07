#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/ether.h>

#include <sys/ioctl.h>             // For interface flags change

#include <netinet/in.h>            // Basic address related functions and types
#include <netinet/ether.h>
#include <net/if.h>
#include <fcntl.h>

#include <netpacket/packet.h>

#include "util.h"
#include "debug.h"

int RemoveElement(void *array, size_t size, int length, int index) {
	assert(array);
	assert(size > 0);
	assert(length > 0);
	assert(index < length);

	uint8_t *arr = array;
	memcpy(&arr[index], &arr[length], size);
	memset(&arr[length], 0, size);

	return length - 1;
}

#define PERIODS_COUNT(i, period) (i / period + i % period ? 1 : 0)
void *Reallocate(void *array, size_t size, int length, int index, int period) {
	assert(size > 0);
	assert(length >= 0);
	assert(index >= 0);
	assert(period > 0);

	int allocated_periods = PERIODS_COUNT(length, period);
	int requested_periods = PERIODS_COUNT(index, period);

	if (requested_periods > allocated_periods) {
		int newSize = size * requested_periods * period;
		if (array == NULL) {
			array = malloc(newSize);
		} else {
			array = realloc(array, newSize);
		}
	}

	return array;
}


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
	struct sockaddr_ll address = {0};

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
		DERROR("Bind socket to interface failed: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int setNonblocking(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		flags = 0;

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void AddToBuffer(uint8_t **bufpointer, void *data, size_t ofData) {
	memcpy(*bufpointer, data, ofData);
	*bufpointer += ofData;
}


void AddToBuffer32(uint8_t **bufpointer, uint32_t data) {
	**((uint32_t **)bufpointer) = htonl(data);
	*bufpointer += 4;
}

void AddToBuffer16(uint8_t **bufpointer, uint16_t data) {
	**((uint16_t **)bufpointer) = htons(data);
	*bufpointer += 2;
}

void AddToBuffer32NoOrder(uint8_t **bufpointer, uint32_t data) {
	**((uint32_t **)bufpointer) = data;
	*bufpointer += 4;
}

void AddToBuffer16NoOrder(uint8_t **bufpointer, uint16_t data) {
	**((uint32_t **)bufpointer) = data;
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

void GetFromBuffer(uint8_t **bufpointer, void *data, size_t ofData) {
	memcpy(data, *bufpointer, ofData);
	*bufpointer += ofData;
}

uint32_t GetFromBuffer32(uint8_t **bufpointer) {
	uint32_t data = **(uint32_t **)bufpointer;
	*bufpointer += 4;
	return ntohl(data);
}

uint16_t GetFromBuffer16(uint8_t **bufpointer) {
	uint16_t data = **(uint16_t **)bufpointer;
	*bufpointer += 2;
	return ntohs(data);
}

uint32_t GetFromBuffer32NoOrder(uint8_t **bufpointer) {
	uint32_t data = **((uint32_t **) bufpointer);
	*bufpointer += 4;
	return data;
}

uint16_t GetFromBuffer16NoOrder(uint8_t **bufpointer) {
	uint16_t data = **((uint16_t **) bufpointer);
	*bufpointer += 2;
	return data;
}

uint8_t GetFromBuffer8(uint8_t **bufpointer) {
	uint8_t data = **bufpointer;
	*bufpointer += 1;
	return data;
}

void GetFromBuffer4(uint8_t **bufpointer, uint8_t *data8, uint8_t *data4) {
	if (data8 != NULL) {
		*data8 = **bufpointer >> 4;
	}
	if (data4 != NULL) {
		*data4 = **bufpointer & 0xF0;
	}
	*bufpointer += 1;
}

uint8_t GetFromBuffer1(const uint8_t *bufpointer, int place) {
	return (*bufpointer >> place) & 0xFE;
}


uint8_t hex_to_uint8(const char hex) {
	uint8_t result;

	switch (hex) {
	case '0': result =  0; break;
	case '1': result =  1; break;
	case '2': result =  2; break;
	case '3': result =  3; break;
	case '4': result =  4; break;
	case '5': result =  5; break;
	case '6': result =  6; break;
	case '7': result =  7; break;
	case '8': result =  8; break;
	case '9': result =  9; break;
	case 'A': result = 10; break;
	case 'B': result = 11; break;
	case 'C': result = 12; break;
	case 'D': result = 13; break;
	case 'E': result = 14; break;
	case 'F': result = 15; break;
	default : result =  0; break;
	}

	return result;
}

uint32_t hex_to_uint32(const char *hex) {
	uint8_t a[] = {
		(hex_to_uint8(hex[0]) << 4) | hex_to_uint8(hex[1]),
		(hex_to_uint8(hex[2]) << 4) | hex_to_uint8(hex[3]),
		(hex_to_uint8(hex[4]) << 4) | hex_to_uint8(hex[5]),
		(hex_to_uint8(hex[6]) << 4) | hex_to_uint8(hex[7])
	};
	uint32_t *result = (uint32_t *) a;
	return *result;
}

