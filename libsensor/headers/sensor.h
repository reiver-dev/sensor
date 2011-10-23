#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>



//RFC XXX
struct ethernet{
		uint8_t destHost[6];
		uint8_t sourceHost[6];
		uint16_t type;
};


//RFC 791
struct ip4Header {
	uint8_t  version;
	uint8_t  headerLength;    //todo check if can combine version and HL as in RFC
	uint8_t  typeOfService;
	uint16_t totalLength;
	uint16_t identification;
	uint16_t offset;          //first 3 bits are flags
	uint8_t  timeToLive;
	uint8_t  protocol;
	struct in_addr sourceAddress;   //all are uint32_t
	struct in_addr destAddress;
};


//RFC 793
struct tcpHeader {
	uint16_t sourcePort;
	uint16_t destPort;
	uint32_t sequenceNumber;
	uint32_t ackNumber;
	uint8_t  dataOffset;
	uint8_t  flags;
	uint16_t window;
	uint16_t checksum;
	uint16_t urgentPointer;
};

//RFC 768
struct udpHeader {
	uint16_t sourcePort;
	uint16_t destPort;
	uint16_t length;
	uint16_t checksum;
};



#endif /*SENSOR_H*/
