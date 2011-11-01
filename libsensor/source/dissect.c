
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/ether.h>

#include "dissect.h"

#define PARSE_BUF_LENGTH 128
#define FULL_BUF_LENGTH 128*4
static char parse_buf[PARSE_BUF_LENGTH];
static char result_buf[FULL_BUF_LENGTH];



static inline void appendprotocol(char *value){
	strcat(result_buf, value);
}



//TODO: PROPER DISSECTION


//--------------actual-dissection-----------------
int sensor_dissect_simple(Queue_t *in, Queue_t *out){
	memset(result_buf, '\0', PARSE_BUF_LENGTH);

	queue_item_t* item = queue_pop(in);
	uint8_t* next_payload = item->content;

	struct ether_header *ethernet = (struct ether_header*) (next_payload);
	appendprotocol(dissect_ethernet(ethernet));

	next_payload += sizeof(struct ether_header);

	switch (ethernet->ether_type) {
		struct iphdr *ipheader;

		case 8:
			ipheader = (struct iphdr*) (next_payload);
			next_payload += ipheader->ihl * 4; // get header length in 4-byte words

			switch (ipheader->protocol){
				struct tcphdr *tcpheader;
				struct udphdr *udpheader;
				struct icmphdr *icmpheader;

				case IPPROTO_TCP:
					tcpheader = (struct tcphdr*) (next_payload);
					next_payload += sizeof(tcpheader);
					break;
				case IPPROTO_UDP:
					udpheader = (struct udphdr*) (next_payload);
					next_payload += sizeof(udpheader);
					break;
				case IPPROTO_ICMP:
					icmpheader = (struct icmphdr*) (next_payload);
					next_payload += sizeof(icmpheader);
					break;

			}
			break;
		//-------------------------
		case ETHERTYPE_ARP:
			break;
	}
	queue_push_copy(out, (uint8_t*)result_buf,strlen(result_buf));
	queue_item_destroy(item);
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
	static char temp_buf[16];
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
			inet_ntop(AF_INET, &header->daddr, temp_buf, 16),
			inet_ntop(AF_INET, &header->saddr, temp_buf, 16),
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

	strcat(parse_buf,"[UDP]\n");
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
