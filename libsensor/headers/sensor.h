#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>



//RFC XXX
struct EthernetHeader{
		uint8_t destHost[6];
		uint8_t sourceHost[6];
		uint16_t type;
};

struct Icmp{
	uint8_t  type;
	uint8_t  code;
	uint16_t checksum;
	union {
		uint32_t u32[1];
		uint16_t u16[2];
		uint8_t  u8[4];
	} data;
};


//RFC 791
#define IP4_VERSION(ipversion) (ipversion >> 4)
#define IP4_HEADER(ipheader)   (ipheader & 0x0f)
struct Ip4Header {
	uint8_t  version_headerLen;
	uint8_t  typeOfService;
	uint16_t totalLength;
	uint16_t identification;
	uint16_t  offset;                //first 3 bits are flags
	uint8_t  timeToLive;
	uint8_t  protocol;
	uint16_t checksum;
	struct in_addr sourceAddress;   //all are uint32_t
	struct in_addr destAddress;
};


//RFC 793
#define TCP_RESERVED(offset) (offset >> 4)
struct TcpHeader {
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
struct UdpHeader {
	uint16_t sourcePort;
	uint16_t destPort;
	uint16_t length;
	uint16_t checksum;
};


struct Packet{
	uint16_t lenght;
	uint16_t payloadLength;
	uint8_t channelProtocol;
	uint8_t netProtocol;
	uint8_t transportProtocol;
	uint8_t *channelHeader;
	uint8_t *netHeader;
	uint8_t *transportHeader;
	uint8_t *barePacket;
	uint8_t *payload;
};

int create_socket();
int close_socket(int socket);
int set_iface_promiscuous(int sock, const char* interfaceName, bool state);
int get_next_packet(int sock, int seconds);
uint8_t dissect(uint8_t* packet, int length);

#endif /*SENSOR_H*/
