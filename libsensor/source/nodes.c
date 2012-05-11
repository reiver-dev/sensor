#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <netinet/in.h>
#include <netinet/ether.h>

#include "sensor.h"
#include "nodes.h"
#include "util.h"
#include "debug.h"
#include "arraylist.h"


/* Locals */
static struct Node *Nodes;
static long NodeCount;

static struct CurrentAddress *current;


static uint32_t get_node_index(uint32_t ip) {
	uint32_t ind = ntohl(ip) - ntohl(current->ip4addr & current->netmask) - 1;
	return ind;
}

/* Main functions */
void nodes_init(struct CurrentAddress *curr) {
	/* memorize current addreses */

	current = curr;

	NodeCount = (1 << (32 - bitcount(current->netmask))) - 2;
	DNOTIFY("Node count is: %i\n", NodeCount);
	assert(NodeCount > 0);
	Nodes = malloc(NodeCount * sizeof(*Nodes));
	memset(Nodes, '\0', NodeCount);

	uint32_t network = ntohl(current->ip4addr & current->netmask);

	for (uint32_t i = 0; i < NodeCount; i++) {
		struct Node *node = & Nodes[i];
		node->ip4addr    = htonl(network + i + 1);
		memset(node->hwaddr, 0, ETH_ALEN);
		node->last_check = 0;
	}

}

void nodes_destroy() {
	free(Nodes);
	NodeCount = 0;
}


void node_answered(uint32_t ip4, uint8_t *hw) {

	struct Node *node = nodes_get_node(ip4);

	DINFO("Node last check was: %i\n", node->last_check);

	if (node->is_online) {
		DINFO("Node (%s) is still online\n", Ip4ToStr(node->ip4addr));
	} else {
		memcpy(node->hwaddr, hw, ETH_ALEN);
	}
	node->last_check = time(0);
}


struct Node *nodes_get_node(uint32_t ip) {
	uint32_t index = get_node_index(ip);
	if (index < 0 || index > NodeCount) {
		DERROR("Node with address %s not found\n", Ip4ToStr(ip));
		return NULL;
	}
	return &Nodes[index];
}

struct Node *nodes_get_destination(uint32_t ip) {
	uint32_t index = get_node_index(ip);
	if (index < 0 || index > NodeCount) {
		if (current->gateway)
			index = get_node_index(current->gateway);
		else
			return NULL;
	}
	return &Nodes[index];
}

bool nodes_is_me(struct Node *node) {
	if (node->ip4addr == current->ip4addr) {
		return true;
	}
	return false;
}

struct Node *nodes_get_me() {
	return nodes_get_node(current->ip4addr);
}

struct Node *nodes_get_gateway() {
	return nodes_get_node(current->gateway);
}

int nodes_count() {
	return NodeCount;
}

struct Node *nodes_get() {
	return Nodes;
}
