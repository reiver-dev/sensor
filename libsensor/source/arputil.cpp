#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "arputil.hpp"


static const uint8_t EtherBroadcast[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void arp_reply_create(uint8_t buffer[ARP_IP4_SIZE], uint32_t my_ip4, const uint8_t my_hw[ETH_ALEN], uint32_t to_ip4, const uint8_t to_hw[ETH_ALEN]) {
	assert(buffer);
	assert(my_ip4 != to_ip4);

	struct ether_header *ethernet;
 	struct arphdr *header;
 	struct arp_ip4 *data;

 	ethernet = (struct ether_header *)buffer;
 	header = (struct arphdr *)(buffer + sizeof(struct ether_header));
 	data = (struct arp_ip4 *)(buffer + sizeof(struct ether_header) + sizeof(struct arphdr));

	memcpy(ethernet->ether_dhost, to_hw, ETH_ALEN);
	memcpy(ethernet->ether_shost, my_hw, ETH_ALEN);
	ethernet->ether_type = htons(ETH_P_ARP);

	header->ar_hrd = htons(ARPHRD_ETHER);
	header->ar_pro = htons(ETH_P_IP);
	header->ar_hln = ETH_ALEN;
	header->ar_pln = 4;
	header->ar_op  = htons(ARPOP_REPLY);

	memcpy(data->ar_sha, my_hw, ETH_ALEN);
	data->ar_sip = my_ip4;
	memcpy(data->ar_tha, to_hw, ETH_ALEN);
	data->ar_tip = to_ip4;

}

void arp_request_create(uint8_t buffer[ARP_IP4_SIZE], uint32_t my_ip4, const uint8_t my_hw[ETH_ALEN], uint32_t to_ip4) {

	struct ether_header *ethernet;
	struct arphdr *header;
	struct arp_ip4 *data;

	ethernet = (struct ether_header *) buffer;
	header = (struct arphdr *) (buffer + sizeof(struct ether_header));
	data = (struct arp_ip4 *) (buffer + sizeof(struct ether_header) + sizeof(struct arphdr));

	memcpy(ethernet->ether_dhost, EtherBroadcast, ETH_ALEN);
	memcpy(ethernet->ether_shost, my_hw, ETH_ALEN);
	ethernet->ether_type = htons(ETH_P_ARP);

	header->ar_hrd = htons(ARPHRD_ETHER);
	header->ar_pro = htons(ETH_P_IP);
	header->ar_hln = ETH_ALEN;
	header->ar_pln = 4;
	header->ar_op = htons(ARPOP_REQUEST);

	memcpy(data->ar_sha, my_hw, ETH_ALEN);
	data->ar_sip = my_ip4;
	memset(data->ar_tha, 0, ETH_ALEN);
	data->ar_tip = to_ip4;

}

void arp_request_set_to_ip(uint8_t *buffer, uint32_t to_ip4) {
	  struct arp_ip4 *arpheader = (struct arp_ip4 *)(buffer + sizeof(struct ether_header) + sizeof(struct arphdr));
	  arpheader->ar_tip = to_ip4;
}


bool arp_is_reply(const uint8_t *buffer, size_t length, const struct InterfaceInfo *current,
	uint32_t *out_ip4, uint8_t out_hw[ETH_ALEN]) {

	if (length < ARP_IP4_SIZE) {
		return false;
	}

	struct ether_header *ethernet;
	struct arphdr *header;
	struct arp_ip4 *arpheader;

	ethernet = (struct ether_header *) &buffer[0];
	header = (struct arphdr *) (buffer + sizeof(struct ether_header));
	arpheader = (struct arp_ip4 *) (buffer + sizeof(struct ether_header) + sizeof(struct arphdr));

	const uint8_t *hw = current->hw.ether_addr_octet;
	if (!memcmp(ethernet->ether_shost, hw, ETH_ALEN)    /* source mac is not me */
		|| memcmp(ethernet->ether_dhost, hw, ETH_ALEN)  /* dest mac is me       */
		|| ethernet->ether_type != ntohs(ETH_P_ARP)                   /* arp protocol         */
		|| header->ar_op != ntohs(ARPOP_REPLY)                        /* arp reply operation  */
		|| !memcmp(arpheader->ar_sha, hw, ETH_ALEN)     /* source is not me     */
		|| memcmp(arpheader->ar_tha, hw, ETH_ALEN)      /* dest is me           */
		) {

		return false;
	}

	*out_ip4 = arpheader->ar_sip;
	memcpy(out_hw, arpheader->ar_sha, ETH_ALEN);

	return true;
}
