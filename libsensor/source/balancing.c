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

#include "survey.h"
#include "debug.h"
#include "util.h"
#include "nodes.h"



static struct {
	uint32_t ip4;
	uint32_t network;
	uint8_t hw[ETH_ALEN];
} current;

bool is_same_network_ip4(uint32_t ip) {
	return (current.network & ip) == current.network;
}


/* ---------------------------------------------- */
void balancing_init(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[ETH_ALEN]) {
	/* memorize current addreses */
	current.ip4 = ip4addr;
	memcpy(current.hw, hwaddr, ETH_ALEN);
	current.network = ip4addr & netmask;

	nodes_init(ip4addr, netmask);

}

void balancing_survey(int packet_sock) {
	int length;
	uint8_t *survey_buf = survey_packet(&length, 0, current.ip4, current.hw);

	assert(survey_buf != 0);
	assert(length > 0);

	int result;
	int nodeCount = nodes_count();
	struct Node *nodes = nodes_get();
	for (int i = 0; i < nodeCount;  i++) {
		survey_set_target_ip(survey_buf, nodes[i].ip4addr);
		DINFO("Send survey to: %s\n", Ip4ToStr(nodes[i].ip4addr));
		result = send(packet_sock, survey_buf, length, 0);
		if (result == -1) {
			DERROR("%s\n", "Failed to send survey");
		}
	}
}

void balancing_check_response(uint8_t *buffer, int length) {
	survey_process_response(buffer, length);
}




void balancing_destroy() {
	nodes_destroy();
}

