#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/ethernet.h>

#include "nodes.h"
#include "util.h"
#include "debug.h"


#define SURVEY_BUFFER_SIZE 256


#define ETHERNET_LENGTH       sizeof(struct ether_header)
#define ARP_CONST_PART_LENGTH sizeof(struct arphdr)
#define ARP_VAR_PART_LENGTH   (ETH_ALEN * 2 + 4 * 2)
#define ARP_SURVEY_BUF_LENGTH ETHERNET_LENGTH + ARP_CONST_PART_LENGTH + ARP_VAR_PART_LENGTH


/* Protocol types */
struct arp_ip4 {
	struct arphdr header;
    uint8_t  ar_sha[ETH_ALEN];  /* Sender hardware address.     */
    uint32_t ar_sip;            /* Sender IP address.           */
    uint8_t ar_tha[ETH_ALEN];   /* Target hardware address.     */
    uint32_t  ar_tip;           /* Target IP address.           */
 } __attribute__((packed));


static const uint8_t EtherBroadcast[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t *survey_packet(int *out_length, const uint32_t toaddr, const uint32_t current_ip4,
		const uint8_t current_mac[ETH_ALEN]) {

	static uint8_t survey_buf[SURVEY_BUFFER_SIZE];

	struct ether_header *ethernet;
	struct arp_ip4 *arpheader;

	memset(survey_buf, '\0', SURVEY_BUFFER_SIZE);
	ethernet = (struct ether_header *) &survey_buf[0];
	arpheader = (struct arp_ip4 *) &survey_buf[ETHERNET_LENGTH];

	memcpy(ethernet->ether_dhost, EtherBroadcast, ETH_ALEN);
	memcpy(ethernet->ether_shost, current_mac, ETH_ALEN);
	ethernet->ether_type = htons(ETH_P_ARP);

	arpheader->header.ar_hrd = htons(ARPHRD_ETHER);
	arpheader->header.ar_pro = htons(ETH_P_IP);
	arpheader->header.ar_hln = ETH_ALEN;
	arpheader->header.ar_pln = 4;
	arpheader->header.ar_op  = htons(ARPOP_REQUEST);

	memcpy(arpheader->ar_sha, current_mac, ETH_ALEN);
	arpheader->ar_sip = current_ip4;
	/* leave target mac as zeros */
	arpheader->ar_tip = toaddr;

	*out_length = ARP_SURVEY_BUF_LENGTH;
	return survey_buf;

}


void survey_set_target_ip(uint8_t *buffer, uint32_t ip) {
	struct arp_ip4 *arpheader = (struct arp_ip4 *) &buffer[ETHERNET_LENGTH];
	arpheader->ar_tip = ip;
}


bool survey_is_response(const uint8_t *buffer, int length, struct CurrentAddress *current) {
	if (length < ARP_SURVEY_BUF_LENGTH) {
		return false;
	}

	struct ether_header *ethernet;
	ethernet = (struct ether_header *) &buffer[0];

	struct arp_ip4 *arpheader;
	arpheader = (struct arp_ip4 *) &buffer[ETHERNET_LENGTH];


	if (!memcmp(ethernet->ether_shost, current->hwaddr, ETH_ALEN)    /* source mac is not me */
		|| memcmp(ethernet->ether_dhost, current->hwaddr, ETH_ALEN)  /* dest mac is me       */
		|| ethernet->ether_type != ntohs(ETH_P_ARP)                  /* arp protocol         */
		|| arpheader->header.ar_op != ntohs(ARPOP_REPLY)             /* arp reply operation  */
		|| !memcmp(arpheader->ar_sha, current->hwaddr, ETH_ALEN)     /* source is not me     */
		|| memcmp(arpheader->ar_tha, current->hwaddr, ETH_ALEN)      /* dest is me           */
		) {

		return false;
	}

	return true;
}


bool survey_process_response(const uint8_t *buffer, int length, struct CurrentAddress *current) {

	if (!survey_is_response(buffer, length, current)) {
		return false;
	}

	struct arp_ip4 *arpheader;
	arpheader = (struct arp_ip4 *) &buffer[ETHERNET_LENGTH];

	uint32_t ip4 = arpheader->ar_sip;
	uint8_t hw[ETH_ALEN];
	memcpy(hw, arpheader->ar_sha, ETH_ALEN);

	DINFO("Got survey response from: IP4:%s HW:%s\n", Ip4ToStr(ip4), EtherToStr(hw));

	node_answered(ip4, hw);

	return true;

}
