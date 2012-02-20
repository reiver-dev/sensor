
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <arpa/inet.h>
#include <netinet/ether.h>

#include "dissect.h"
#include "sensor.h"
#include "debug.h"

#define PARSE_BUF_LENGTH 196
#define FULL_BUF_LENGTH 196*4
static char parse_buf[PARSE_BUF_LENGTH];
static char content[FULL_BUF_LENGTH];

#define ALLOCATE(ptr, size)                          \
	do {                                             \
		ptr = malloc(sizeof(typeof(*ptr))*size)      \
		memset(ptr, '\0', sizeof(typeof(*ptr))*size) \
	} while(0);                                      \



//TODO: LENGTH SAFE CHECK
#define next_step(current, next, length) curr > length ? dissect_out_of_bounds(), break;

//--------------actual-dissection-----------------
sensor_dissected_t *sensor_dissect_simple(sensor_captured_t *captured){

	//extract from queue
	sensor_dissected_t* result;

	struct ether_header *Ethernet;
	struct iphdr *IP4header;
	struct tcphdr *TCPheader;
	struct udphdr *UDPheader;
	struct icmphdr *ICMPheader;


	uint8_t* packet_begin = captured->buffer;
	int position = 0;

	//allocate dissected
	memset(content, '\0', FULL_BUF_LENGTH);

	//enter base values

	Ethernet = (struct ether_header*) (packet_begin);

	strcat(content,dissect_ethernet(Ethernet));

	position += sizeof(struct ether_header);


	/* content processing */

	uint16_t ethernet_type = ntohs(Ethernet->ether_type);

	switch (ethernet_type) {
	case ETHERTYPE_IP:
		IP4header = (struct iphdr*) (packet_begin + position);
		strcat(content, dissect_ip(IP4header));
		/* CHECK IP BOUNDS*/
		if (position + (IP4header->ihl * 4) <= captured->length) {
			position += IP4header->ihl * 4; // get header length in 4-byte words
		} else {
			strcat(content, dissect_out_of_bounds());
			position += sizeof(struct iphdr);
			break;
		}

		switch (IP4header->protocol) {
		case IPPROTO_TCP:
			TCPheader = (struct tcphdr*) (packet_begin + position);
			strcat(content,dissect_tcp(TCPheader));
			/* CHECK IP BOUNDS */
			if (position + (TCPheader->doff * 4) <= captured->length) {
				position += TCPheader->doff;
			} else {
				strcat(content, dissect_out_of_bounds());
				position += sizeof(struct tcphdr);
			}
			break;

		case IPPROTO_UDP:
			UDPheader = (struct udphdr*) (packet_begin + position);
			strcat(content,dissect_udp(UDPheader));
			position += sizeof(UDPheader);
			break;

		case IPPROTO_ICMP:
			ICMPheader = (struct icmphdr*) (packet_begin + position);
			strcat(content,dissect_icmp(ICMPheader));
			position += sizeof(ICMPheader);
			break;
		}
		break; /* IP4 protocol */

	case ETHERTYPE_ARP:
		break;
	}

//	DINFO("\n%s--------------\n", content);



	//form result

	result = init_dissected(strlen(content), captured->length - position);

	strcpy(result->content, content);

	assert(result->payload_length >= 0);
	if (result->payload_length > 0)
		memcpy(result->payload, packet_begin + position, result->payload_length);
	else
		result->payload = 0;

	strcpy(result->mac_dest, ether_ntoa((struct ether_addr*)Ethernet->ether_dhost));
	strcpy(result->mac_source, ether_ntoa((struct ether_addr*)Ethernet->ether_shost));

	result->timestamp = captured->timestamp;

	destroy_captured(captured);

	return result;
}


//-----------------------------------------------
char* dissect_ethernet(struct ether_header *header){
	char temp_buf_source[18];
	char temp_buf_dest[18];
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH,
			"[Ethernet]\n"
			"SOURCE=%s\n"
			"DESTINATION=%s\n",
			ether_ntoa_r((struct ether_addr*)header->ether_shost, temp_buf_source),
			ether_ntoa_r((struct ether_addr*)header->ether_dhost, temp_buf_dest)
	);
	return parse_buf;
}

char* dissect_ip(struct iphdr *header){
	char temp_buf_source[18];
	char temp_buf_dest[18];
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH,
			"[IP]\n"
			"VERSION=%d\n"
			"HEADER_LENGTH=%d\n"
			"DESTINATION=%s\n"
			"SOURCE=%s\n"
			"TTL=%d\n"
			"TOS=%d\n"
			"IDENTIFICATION=%d\n"
			"DONT_FRAGMENT=%d\n"
			"MORE_FRAGMENTS=%d\n"
			"FRAGMENT_OFFSET=%d\n"
			"CHECKSUM=%d\n",
			header->version,
			header->ihl,
			inet_ntop(AF_INET, &header->daddr, temp_buf_dest, 16),
			inet_ntop(AF_INET, &header->saddr, temp_buf_source, 16),
			header->ttl,
			header->tos,
			ntohl(header->id),
			header->frag_off & 0x4000,    //don't fragment flag
			header->frag_off & 0x2000,    //more fragments flag
			header->frag_off & 0x1fff,    //fragment offset
			ntohs(header->check)
	);
	return parse_buf;
}

char* dissect_tcp(struct tcphdr *header){
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH,
			"[TCP]\n"
			"SOURCE=%d\n"
			"DESTINATION=%d\n"
			"SEQUENCE=%d\n"
			"ACKNOWLEDGMENT=%d\n"
			"DATAOFFSET=%d\n"
			"FIN=%d\n"
			"SYN=%d\n"
			"RST=%d\n"
			"PSH=%d\n"
			"ACK=%d\n"
			"URG=%d\n"
			"WINDOW=%d\n"
			"CHECKSUM=%d\n"
			"URGENTPTR=%d\n",
			ntohs(header->source),
			ntohs(header->dest),
			ntohl(header->seq),
			ntohl(header->ack_seq),
			header->doff,
			header->fin,
			header->syn,
			header->rst,
			header->psh,
			header->ack,
			header->urg,
			ntohs(header->window),
			ntohs(header->check),
			ntohs(header->urg_ptr)
	);
	return parse_buf;
}


char* dissect_udp(struct udphdr *header){
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH,
			"[UDP]\n"
			"SOURCE=%d\n"
			"DESTINATION=%d\n"
			"LENGTH=%d\n"
			"CHECKSUM=%d\n",
			ntohs(header->source),
			ntohs(header->dest),
			ntohs(header->len),
			ntohs(header->check)
	);

	return parse_buf;
}


char* dissect_icmp(struct icmphdr *header){
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);

	snprintf(parse_buf, PARSE_BUF_LENGTH,
				"[ICMP]\n"
				"CODE=%d\n"
				"TYPE=%d\n"
				"CHECKSUM=%d\n",
				header->code,
				header->type,
				ntohs(header->checksum)
	);

	if (header->type == ICMP_ECHO) {
		int length = strlen(parse_buf);
		snprintf(parse_buf + strlen(parse_buf), PARSE_BUF_LENGTH - length,
				"ID=%d\n"
				"SEQUENCE=%d\n",
				ntohs(header->un.echo.id),
				ntohs(header->un.echo.sequence)
		);
	}


	return parse_buf;
}

char* dissect_out_of_bounds(){
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH, "[OUT_OF_BOUNDS]");
	return parse_buf;
}

