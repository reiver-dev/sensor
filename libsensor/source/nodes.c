#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#include <netinet/in.h>
#include <netinet/ether.h>

#include "nodes.h"
#include "util.h"
#include "debug.h"

/* Locals */

static struct Node *Nodes;
static unsigned long NodeCount;


#define MAXOWNED 255
#define MAXSENSORS 255
static struct Node **OwnedNodes;
static struct Node **SensorNodes;
static int SensorCount;
static int OwnedCount;

static struct {
	uint32_t ip4;
	uint32_t network;
	uint8_t hw[ETH_ALEN];
} current;


static uint32_t get_node_index(uint32_t ip) {
	uint32_t ind = ntohl(ip) - ntohl(current.network) - 1;
	return ntohl(ip) < ntohl(current.ip4) ? ind : ind - 1;
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

/*-----------------------------------------------*/
void nodes_init(const uint32_t ip4addr, const uint32_t netmask) {
	/* memorize current addreses */
	current.ip4 = ip4addr;
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

	OwnedNodes = malloc(MAXOWNED * sizeof(void *));
	OwnedCount = 0;

	SensorNodes = malloc(MAXSENSORS * sizeof(void *));
	SensorCount = 0;
}

struct Node *nodes_get() {
	return Nodes;
}

struct Node *node_get(uint32_t ip) {
	uint32_t index = get_node_index(ip);
	return &Nodes[index];
}


int nodes_count() {
	return NodeCount;
}

void nodes_destroy() {
	free(Nodes);
	free(OwnedNodes);
	NodeCount = 0;
}

/*----------------------------------------*/

void node_answered(uint32_t ip4, uint8_t *hw) {

	struct Node *node = node_get(ip4);

	DINFO("Node last check was: %i\n", node->last_check);

	if (node->is_online) {
		DINFO("Node is still online");
	} else {
		node->type = NODE_CLIENT;
		node->is_online = true;
		node->info.client.load = 0;
		node->info.client.type = CLIENT_FREE;
		memcpy(node->hwaddr, hw, ETH_ALEN);
	}

	node->last_check = time(0);
}

void node_as_sensor() {

}
