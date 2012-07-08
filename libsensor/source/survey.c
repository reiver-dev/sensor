#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/ethernet.h>


#include "sensor_private.h"
#include "nodes.h"
#include "debug.h"
#include "util.h"

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


bool survey_is_response(const struct CurrentAddress *current, const uint8_t *buffer, int length) {
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


bool survey_process_response(const struct CurrentAddress *current, const uint8_t *buffer, int length) {

	if (!survey_is_response(current, buffer, length)) {
		return false;
	}

	struct arp_ip4 *arpheader;
	arpheader = (struct arp_ip4 *) &buffer[ETHERNET_LENGTH];

	uint32_t ip4 = arpheader->ar_sip;
	uint8_t hw[ETH_ALEN];
	memcpy(hw, arpheader->ar_sha, ETH_ALEN);

	node_answered(ip4, hw);

	return true;

}

void survey_perform_survey(const struct CurrentAddress *current, int packet_sock) {
	DINFO("%s\n", "Starting survey");

	int length;
	uint8_t *survey_buf = survey_packet(&length, 0, current->ip4addr, current->hwaddr);

	assert(survey_buf != 0);
	assert(length > 0);

	int result;

	int nodeCount = (1 << (32 - bitcount(current->netmask))) - 2;
	uint32_t network = ntohl(current->ip4addr & current->netmask);

	for (int i = 0; i < nodeCount;  i++) {
		uint32_t ip4addr = htonl(network + i + 1);
		if (ip4addr != current->ip4addr) {
			survey_set_target_ip(survey_buf, ip4addr);
			result = send(packet_sock, survey_buf, length, 0);
			if (result == -1) {
				DERROR("Failed to send survey to %s\n", Ip4ToStr(ip4addr));
			}
		}
	}

	DINFO("%s\n", "Survey finished");
}
