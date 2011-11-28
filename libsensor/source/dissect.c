
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/ether.h>

#include "dissect.h"
#include "sensor.h"
#include "debug.h"


#define PARSE_BUF_LENGTH 196
#define FULL_BUF_LENGTH 196*4
static char parse_buf[PARSE_BUF_LENGTH];

//TODO: LENGTH SAFE CHECK
#define next_step(current, next, length) curr > length ? dissect_out_of_bounds(), break;

//--------------actual-dissection-----------------
int sensor_dissect_simple(Queue_t *in, Queue_t *out){
	// extract from queue
	sensor_captured_t* captured = (sensor_captured_t*)queue_pop(in);
	// allocate dissected
	sensor_dissected_t* result = malloc(sizeof(sensor_dissected_t));
	memset(result, '\0', sizeof(sensor_dissected_t));

	// enter base values
	result->timestamp = captured->timestamp;
	result->content_length = FULL_BUF_LENGTH;
	result->content = malloc(FULL_BUF_LENGTH);
	memset(result->content, '\0', FULL_BUF_LENGTH);

	uint8_t* packet_begin = captured->buffer;
	int position = 0;

	struct ether_header *ethernet = (struct ether_header*) (packet_begin);
	strcat(result->mac_dest, ether_ntoa((struct ether_addr*)ethernet->ether_dhost));
	strcat(result->mac_source, ether_ntoa((struct ether_addr*)ethernet->ether_shost));
	strcat(result->content,dissect_ethernet(ethernet));
	position += sizeof(struct ether_header);

	uint16_t ethernet_type = ntohs(ethernet->ether_type);
	switch (ethernet_type) {
		struct iphdr *ipheader;

		case ETHERTYPE_IP:
			ipheader = (struct iphdr*) (packet_begin + position);
			strcat(result->content,dissect_ip(ipheader));
			// CHECK BOUNDS
			if (position + (ipheader->ihl * 4) <= captured->length) {
				position += ipheader->ihl * 4; // get header length in 4-byte words
			} else {
				strcat(result->content, dissect_out_of_bounds());
				position += sizeof(struct iphdr);
				break;
			}

			switch (ipheader->protocol){
				struct tcphdr *tcpheader;
				struct udphdr *udpheader;
				struct icmphdr *icmpheader;

				case IPPROTO_TCP:
					tcpheader = (struct tcphdr*) (packet_begin + position);
					strcat(result->content,dissect_tcp(tcpheader));
					if (position + (tcpheader->doff * 4) <= captured->length) {
						position += tcpheader->doff;
					} else {
						strcat(result->content, dissect_out_of_bounds());
						position += sizeof(struct tcphdr);
					}
					break;
				case IPPROTO_UDP:
					udpheader = (struct udphdr*) (packet_begin + position);
					strcat(result->content,dissect_udp(udpheader));
					position += sizeof(udpheader);
					break;
				case IPPROTO_ICMP:
					icmpheader = (struct icmphdr*) (packet_begin + position);
					strcat(result->content,dissect_icmp(icmpheader));
					position += sizeof(icmpheader);
					break;
			}
			break;
		//-------------------------
		case ETHERTYPE_ARP:
			break;
	}
	DEBUG_PRINTF("%s--------------\n", result->content);
	result->payload_length = captured->length - position;
	if (result->payload_length > 0) {
		result->payload = malloc(result->payload_length);
		memcpy(result->payload, packet_begin, result->payload_length);
	} else {
		result->payload = 0;
	}
	queue_push(out, result);

	free(captured->buffer);
	free(captured);

	return 0;
}

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
			header->id,
			header->frag_off & 0x4000,    //don't fragment flag
			header->frag_off & 0x2000,    //more fragments flag
			header->frag_off & 0x1fff,    //fragment offset
			header->check
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
			header->source,
			header->dest,
			header->seq,
			header->ack_seq,
			header->doff,
			header->fin,
			header->syn,
			header->rst,
			header->psh,
			header->ack,
			header->urg,
			header->window,
			header->check,
			header->urg_ptr
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
			header->source,
			header->dest,
			header->len,
			header->check
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
				header->checksum
	);

	if (header->type == ICMP_ECHO) {
		int length = strlen(parse_buf);
		snprintf(parse_buf + strlen(parse_buf), PARSE_BUF_LENGTH - length,
				"ID=%d\n"
				"SEQUENCE=%d\n",
				header->un.echo.id,
				header->un.echo.sequence
		);
	}


	return parse_buf;
}

char* dissect_out_of_bounds(){
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH, "[OUT_OF_BOUNDS]");
	return parse_buf;
}

