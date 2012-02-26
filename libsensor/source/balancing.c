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


#define NODE_UNKNOWN 0
#define NODE_SENSOR 1
#define NODE_CLIENT 2

#define CLIENT_FREE 0
#define CLIENT_FOREIGN 1
#define CLIENT_OWNED 2


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

static struct Node *Nodes;
static unsigned long NodeCount;


#define MAXOWNED 255
#define MAXSENSORS 255
static struct Node **OwnedNodes;
static int OwnedCount;


static struct {
	uint32_t ip4;
	uint32_t network;
	uint8_t hw[ETH_ALEN];
} current;

bool is_same_network_ip4(uint32_t ip) {
	return (current.network & ip) == current.network;
}


uint32_t get_node_index(uint32_t ip) {
	uint32_t ind = ntohl(ip) - ntohl(current.network) - 1;
	return ntohl(ip) < ntohl(current.ip4) ? ind : ind - 1;
}
/* ----- spoofing ----- */

int take_ownership(int index) {
	assert(index >= 0);         /* in array bounds   */
	assert(index < NodeCount);  /* in array bounds   */
	assert(index > OwnedCount); /* not already owned */

	if (index > MAXOWNED) {
		return 1; /* max owned reached */
	}

	struct Node *node = &Nodes[index];
	assert(node->is_online);

	if (node->type == NODE_SENSOR) {
		DWARNING("Tried to take ownership over sensor: IP4 = %s", Ip4ToStr(node->ip4addr));
		return 1;
	}

	node->info.client.type = CLIENT_OWNED;

	OwnedNodes[OwnedCount] = node;
	OwnedCount++;

	return 0;

}

int get_owned_index(struct Node *node) {
	int i;
	for (i = 0; i < OwnedCount; i ++) {
		if (OwnedNodes[i] == node) {
			return i;
		}
	}
	return -1;
}

int release_ownership(int index) {
	assert(index >= 0);         /* in array bounds   */
	assert(index < NodeCount);  /* in array bounds   */
	assert(index < OwnedCount); /* already owned     */

	struct Node *node = &Nodes[index];

	if (node->type != NODE_CLIENT && node->info.client.type != CLIENT_OWNED) {
		DWARNING("Tried to release not owned node: IP4 = %s", Ip4ToStr(node->ip4addr));
		return 1;
	}

	int owned = get_owned_index(node);
	assert(owned >= 0);

	node->info.client.type = CLIENT_FREE;

	if (OwnedCount - 1 > 0 && OwnedCount - 1 != owned) {
		/* fast delete */
		OwnedNodes[owned] = OwnedNodes[OwnedCount - 1];
		OwnedNodes[OwnedCount - 1] = 0;
	} else {
		OwnedNodes[owned] = 0;
	}
	OwnedCount--;

	return 0;
}




/* ---------------------------------------------- */
void balancing_initNodes(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[ETH_ALEN]) {
	/* memorize current addreses */
	current.ip4 = ip4addr;
	memcpy(current.hw, hwaddr, ETH_ALEN);
	current.network = ip4addr & netmask;

	NodeCount = (1 << (32 - bitcount(netmask))) - 2;
	Nodes = malloc(NodeCount * sizeof(*Nodes));
	memset(Nodes, '\0', NodeCount);

	uint32_t network = ntohl(current.network);
	DNOTIFY("Node count is: %i\n", NodeCount);
	uint32_t tmp;
	uint32_t curr_ip = ntohl(current.ip4);
	for (uint32_t i = 0; i < NodeCount; i++) {
		struct Node *node = & Nodes[i];
		tmp = network + i + 1;
		node->ip4addr = htonl(tmp < curr_ip ? tmp : tmp + 1);
		node->type    = NODE_UNKNOWN;
	}

	OwnedNodes = malloc(MAXOWNED * sizeof(int));
	OwnedCount = 0;
}

void balancing_survey(int packet_sock) {
	int length;
	uint8_t *survey_buf = survey_packet(&length, 0, current.ip4, current.hw);

	assert(survey_buf != 0);
	assert(length > 0);

	int result;
	for (int i = 0; i < NodeCount;  i++) {
		survey_set_target_ip(survey_buf, Nodes[i].ip4addr);
		DINFO("Send survey to: %s\n", Ip4ToStr(Nodes[i].ip4addr));
		result = send(packet_sock, survey_buf, length, 0);
		if (result == -1) {
			DERROR("%s\n", "Failed to send survey");
		}
	}
}

void balancing_check_response(uint8_t *buffer, int length) {
	uint32_t ip4 = 0;
	uint8_t hw[ETH_ALEN] = {0};

	if(!survey_extract_response(buffer, length, &ip4, hw)) {
		if (is_same_network_ip4(ip4)) {
			DINFO("Got survey response from: IP4:%s HW:%s\n", Ip4ToStr(ip4), EtherToStr(hw));
			uint32_t index = get_node_index(ip4);
			DINFO("Node last check was: %i\n", Nodes[index].last_check);
			DINFO("Node was: %s\n", Nodes[index].is_online ? "ONLINE" : "OFFLINE");
			DINFO("Node type was: %i\n", Nodes[index].is_online);
			Nodes[index].last_check = time(0);
			Nodes[index].type = NODE_CLIENT;
			Nodes[index].is_online = true;
			Nodes[index].info.client.load = 0;
			Nodes[index].info.client.type = CLIENT_FREE;
			memcpy(Nodes[index].hwaddr, hw, ETH_ALEN);
		}
	}

}


void balancing_destroyNodes() {
	free(Nodes);
	free(OwnedNodes);
	NodeCount = 0;
}

