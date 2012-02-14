#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <netinet/in.h>
#include <netinet/ether.h>

#include <arpa/inet.h>

#include "debug.h"

#define ARP_CONST_PART_LENGTH 8
#define ARP_VAR_PART_LENGTH   20
#define ARP_VAR_PART_START    ETH_ALEN + ARP_CONST_PART_LENGTH
#define ARP_SURVEY_BUF_LENGTH ETH_ALEN + ARP_CONST_PART_LENGTH + ARP_VAR_PART_LENGTH

#define NODE_UNKNOWN 0
#define NODE_SENSOR 1
#define NODE_CLIENT 2

#define CLIENT_FREE 0
#define CLIENT_FOREIGN 1
#define CLIENT_MY 2

#define SURVEY_BUFFER_SIZE 256


/* Protocol types */
struct arp_ip4 {
    uint8_t  ar_sha[ETH_ALEN];  /* Sender hardware address.     */
    uint32_t ar_sip;            /* Sender IP address.           */
    uint8_t ar_tha[ETH_ALEN];   /* Target hardware address.     */
    uint32_t  ar_tip;           /* Target IP address.           */
 };



/* Types */
struct Node_sensor {
	char id[16];
	int clients_count;
};

struct Node_client {
	int type;
	int load;
};

union node_info {
	struct Node_client client;
	struct Node_sensor sensor;
};

struct Node {
	uint32_t ip4addr;
	uint8_t  hwaddr[ETH_ALEN];
	int      type;
	time_t   last_check;
	bool     is_online;

	union node_info info;
};

/* Locals */

static uint8_t ether_broadcast[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static struct Node *nodes;
static unsigned long node_count;

static uint8_t survey_buf[SURVEY_BUFFER_SIZE];

static struct {
	uint32_t ip4;
	uint32_t network;
	uint8_t hw[ETH_ALEN];
} current;



/* HAKMEM BITCOUNT */
int bitcount(unsigned int n) {
	unsigned int count;
	count = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
	return ((count + (count >> 3)) & 030707070707) % 63;
}


void create_survey_packet_arp_ip4(uint8_t *buffer, const uint32_t toaddr, const uint32_t current_ip4,
		const uint8_t current_mac[ETH_ALEN]) {

	assert(SURVEY_BUFFER_SIZE > ARP_SURVEY_BUF_LENGTH);

	struct ether_header *ethernet;
	struct arphdr *arpheader;
	struct arp_ip4 *arp_addreses;

	memset(survey_buf, '\0', SURVEY_BUFFER_SIZE);
	ethernet = (struct ether_header *) &buffer[0];
	arpheader = (struct arphdr *) &buffer[ETH_ALEN];
	arp_addreses = (struct arp_ip4 *) &buffer[ARP_VAR_PART_START];

	memcpy(ethernet->ether_dhost, ether_broadcast, ETH_ALEN);
	memcpy(ethernet->ether_shost, current_mac, ETH_ALEN);
	ethernet->ether_type = htons(ETH_P_ARP);

	arpheader->ar_hrd = htons(ARPHRD_ETHER);
	arpheader->ar_pro = htons(ETH_P_IP);
	arpheader->ar_hln = ETH_ALEN;
	arpheader->ar_pln = 4;
	arpheader->ar_op  = htons(ARPOP_REQUEST);

	memcpy(arp_addreses->ar_sha, current_mac, ETH_ALEN);
	arp_addreses->ar_sip = current_ip4;
	/* leave target mac as zeros */
	arp_addreses->ar_tip = toaddr;

}

void replace_target_ip4_in_arp(uint8_t *buffer, uint32_t ip) {
	struct arp_ip4 *arp_addreses = (struct arp_ip4 *) &buffer[ARP_VAR_PART_START];
	arp_addreses->ar_tip = ip;
}


bool is_same_network_ip4(uint32_t ip) {
	return (current.network & ip) == current.network;
}


uint64_t get_node_index(uint32_t ip) {
	return (current.network ^ ip) - 1;
}



void balancing_initNodes(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[ETH_ALEN]) {


	current.ip4 = ip4addr;
	memcpy(current.hw, hwaddr, ETH_ALEN);
	uint64_t i;
	uint32_t network = ip4addr & netmask;

	node_count = (1 << (32 - bitcount(netmask))) - 2;

	nodes = malloc(node_count * sizeof(*nodes));
	memset(nodes, '\0', node_count);

	for (i = 0; i < node_count; i++) {
		struct Node *node = & nodes[i];
		node->ip4addr = network + (i+1);
		node->type    = NODE_UNKNOWN;
	}

}

void balancing_survey(int packet_sock) {

	create_survey_packet_arp_ip4(survey_buf, 0, current.ip4, current.hw);
	for (int i = 0; i < node_count;  i++) {
		replace_target_ip4_in_arp(survey_buf, nodes[i].ip4addr);
		send(packet_sock, survey_buf, SURVEY_BUFFER_SIZE, 0);
	}

}

void balancing_check_response(const uint8_t *buffer, int length) {

	if (length < ARP_SURVEY_BUF_LENGTH) {
		return;
	}

	struct ether_header *ethernet;
	struct arphdr *arpheader;
	struct arp_ip4 *arp_addreses;

	ethernet = (struct ether_header *) &buffer[0];
	if (!memcpy(ethernet->ether_dhost, current.hw, ETH_ALEN) || ethernet->ether_type != ntohs(ETH_P_ARP)) {
		return;
	}

	DNOTIFY("Received ARP response from: %s\n", ether_ntoa((struct ether_addr*)ethernet->ether_dhost));

	arpheader = (struct arphdr *) &buffer[ETH_ALEN];
	arp_addreses = (struct arp_ip4 *) &buffer[ARP_VAR_PART_START];


	if (arpheader->ar_op == ARPOP_REPLY
	    && !memcmp(arp_addreses->ar_tha, current.hw, ETH_ALEN)
	    && arp_addreses->ar_tip == current.ip4
	    && is_same_network_ip4(arp_addreses->ar_sip))
	{
		uint64_t index = get_node_index(arp_addreses->ar_sip);

		nodes[index].last_check = time(0);
		nodes[index].type       = NODE_CLIENT;
		nodes[index].is_online  = true;
		nodes[index].info.client.load = 0;
		nodes[index].info.client.type = CLIENT_FREE;
		memcpy(nodes[index].hwaddr, arp_addreses->ar_sha, ETH_ALEN);

		DNOTIFY("Found host: IP4 = %s\tMAC = %s\n",
				inet_ntoa(*(struct in_addr *)&arp_addreses->ar_sip),
				ether_ntoa((struct ether_addr*)arp_addreses->ar_sha));
	}

}


void balancing_destroyNodes() {
	free(nodes);
	node_count = 0;
}

