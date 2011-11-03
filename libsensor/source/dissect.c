
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/ether.h>

#include "dissect.h"
#include "sensor.h"



#define PARSE_BUF_LENGTH 128
#define FULL_BUF_LENGTH 128*4
static char parse_buf[PARSE_BUF_LENGTH];

//TODO: PROPER DISSECTION


//--------------actual-dissection-----------------
int sensor_dissect_simple(Queue_t *in, Queue_t *out){
	sensor_captured_t* captured = (sensor_captured_t*)queue_pop(in);

	sensor_dissected_t* result = malloc(sizeof(sensor_dissected_t));
	memset(result, '\0', sizeof(sensor_dissected_t));

	result->timestamp = captured->timestamp;
	result->content_length = FULL_BUF_LENGTH;
	result->content = malloc(FULL_BUF_LENGTH);
	memset(result->content, '\0', FULL_BUF_LENGTH);

	uint8_t* next_payload = captured->buffer;

	struct ether_header *ethernet = (struct ether_header*) (next_payload);
	strcat(result->mac_dest, ether_ntoa((struct ether_addr*)ethernet->ether_dhost));
	strcat(result->mac_source, ether_ntoa((struct ether_addr*)ethernet->ether_shost));
	strcat(result->content,dissect_ethernet(ethernet));
	next_payload += sizeof(struct ether_header);

	switch (ethernet->ether_type) {
		struct iphdr *ipheader;

		case 8:
			ipheader = (struct iphdr*) (next_payload);
			strcat(result->content,dissect_ip(ipheader));
			next_payload += ipheader->ihl * 4; // get header length in 4-byte words

			switch (ipheader->protocol){
				struct tcphdr *tcpheader;
				struct udphdr *udpheader;
				struct icmphdr *icmpheader;

				case IPPROTO_TCP:
					tcpheader = (struct tcphdr*) (next_payload);
					strcat(result->content,dissect_tcp(tcpheader));
					next_payload += sizeof(tcpheader);
					break;
				case IPPROTO_UDP:
					udpheader = (struct udphdr*) (next_payload);
					strcat(result->content,dissect_udp(udpheader));
					next_payload += sizeof(udpheader);
					break;
				case IPPROTO_ICMP:
					icmpheader = (struct icmphdr*) (next_payload);
					strcat(result->content,dissect_icmp(icmpheader));
					next_payload += sizeof(icmpheader);
					break;
			}
			break;
		//-------------------------
		case ETHERTYPE_ARP:
			break;
	}
	result->payload_length = captured->length - (next_payload - captured->buffer);
	result->payload = malloc(result->payload_length);
	memcpy(result->payload, next_payload, result->payload_length);

	queue_push(out, result);

	free(captured->buffer);
	free(captured);

	return 0;
}

char* dissect_ethernet(struct ether_header *header){
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH,
			"[Ethernet]\n"
			"DESTINATION=%s\n"
			"SOURCE=%s\n",
			ether_ntoa((struct ether_addr*)header->ether_dhost),
			ether_ntoa((struct ether_addr*)header->ether_shost)
	);
	return parse_buf;
}

char* dissect_ip(struct iphdr *header){
	char temp_buf_source[16];
	char temp_buf_dest[16];
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH,
			"[IP]\n"
			"VERSION=%d\n"
			"HEADER_LENGTH=%d\n"
			"DESTINATION=%s\n"
			"SOURCE=%s\n"
			"TTL=%d\n"
			"TOS=%d\n",
			header->version,
			header->ihl,
			inet_ntop(AF_INET, &header->daddr, temp_buf_dest, 16),
			inet_ntop(AF_INET, &header->saddr, temp_buf_source, 16),
			header->ttl,
			header->tos
	);
	return parse_buf;
}

char* dissect_tcp(struct tcphdr *header){
	memset(parse_buf, '\0', PARSE_BUF_LENGTH);
	snprintf(parse_buf, PARSE_BUF_LENGTH,
			"[TCP]\n"
			"SOURCE=%d\n"
			"DESTINATION=%d\n",
			header->source,
			header->dest
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
	return parse_buf;
}
