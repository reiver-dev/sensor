#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include <arpa/inet.h>
#include <netinet/ether.h>

#include "util.h"
#include "debug.h"

char *Ip4ToStr(const uint32_t ip) {
	return inet_ntoa(*(struct in_addr *)&ip);
}

char *EtherToStr(const uint8_t eth[ETH_ALEN]) {
	return ether_ntoa((struct ether_addr*)eth);
}

char *Time4ToStr(const time_t t) {
	static char str[16] = {0};
	strftime(str, 16, "%T", localtime(&t));
	return str;
}

int bitcount(unsigned int n) {
	unsigned int count;
	count = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
	return ((count + (count >> 3)) & 030707070707) % 63;
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


static uint8_t combine(uint8_t half1, uint8_t half2) {
	return half1 << 4 | half2;
}
uint32_t hex_to_uint32(const char *hex) {
	uint8_t a[] = {
		combine(hex_to_uint8(hex[0]), hex_to_uint8(hex[1])),
		combine(hex_to_uint8(hex[2]), hex_to_uint8(hex[3])),
		combine(hex_to_uint8(hex[4]), hex_to_uint8(hex[5])),
		combine(hex_to_uint8(hex[6]), hex_to_uint8(hex[7]))
	};
	uint32_t *result = (uint32_t *) a;
	return *result;
}

