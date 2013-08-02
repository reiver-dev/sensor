#ifndef ARPUTIL_H_
#define ARPUTIL_H_

#include <netinet/ether.h>
#include <netinet/in.h>

#include "netinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol types */
struct arp_ip4 {
    uint8_t  ar_sha[ETH_ALEN];  /* Sender hardware address.     */
    uint32_t ar_sip;            /* Sender IP address.           */
    uint8_t ar_tha[ETH_ALEN];   /* Target hardware address.     */
    uint32_t  ar_tip;           /* Target IP address.           */
 } __attribute__((packed));

#define ARP_IP4_SIZE (sizeof(struct ether_header) + sizeof(struct arphdr) + sizeof(struct arp_ip4))

void arp_reply_create(uint8_t buffer[ARP_IP4_SIZE], uint32_t my_ip4,
	const uint8_t my_hw[ETH_ALEN], uint32_t to_ip4,
	const uint8_t to_hw[ETH_ALEN]);

void arp_request_create(uint8_t buffer[ARP_IP4_SIZE], uint32_t my_ip4,
	const uint8_t my_hw[ETH_ALEN], uint32_t to_ip4);

void arp_request_set_to_ip(uint8_t *buffer, uint32_t to_ip4);

bool arp_is_reply(const uint8_t *buffer, size_t length,
	const uint8_t current_hw[ETH_ALEN],
	uint32_t *out_ip4, uint8_t out_hw[ETH_ALEN]);

#ifdef __cplusplus
}
#endif

#endif /* ARPUTIL_H_ */
