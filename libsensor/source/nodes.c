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
#include "hashmap.h"


/* Locals */
static struct Node *Me;
static HashMap Nodes;

static struct CurrentAddress *current;

static bool is_same_network_ip4(uint32_t ip) {
	uint32_t network = current->ip4addr & current->netmask;
	return (network & ip) == network;
}

/* Main functions */
void nodes_init(struct CurrentAddress *curr) {
	/* memorize current addreses */

	current = curr;
	Me = malloc(sizeof(struct Node));
	Me->ip4addr = current->ip4addr;
	memcpy(Me->hwaddr, current->hwaddr, ETH_ALEN);
	Me->owned_by = NULL;
	Me->load = 0;
	Me->current_load = NODE_LOAD_NOT_READY;
	Me->last_check = 0;
	Me->is_online = true;

	Nodes = HashMap_initInt32(free, free);

}

void nodes_destroy() {
	HashMap_destroy(Nodes);
	free(Me);
}

size_t nodes_count() {
	return HashMap_size(Nodes);
}

void node_answered(uint32_t ip4, uint8_t *hw) {

	struct Node *node = nodes_get_node(ip4);

	if (node) {
		node->last_check = time(NULL);
	} else {
		DINFO("Node found (%s)\n", Ip4ToStr(ip4));
		node = malloc(sizeof(struct Node));
		node->ip4addr = ip4;
		memcpy(node->hwaddr, hw, ETH_ALEN);
		node->is_online = true;
		node->load = 0;
		node->current_load = 0;
		node->owned_by = NULL;
		node->last_check = time(NULL);
		HashMap_addInt32(Nodes, ip4, node);
	}

}


struct Node *nodes_get_node(uint32_t ip) {
	return HashMap_get(Nodes, &ip);
}

void nodes_remove(uint32_t ip) {
	HashMap_remove(Nodes, &ip);
}

struct Node *nodes_get_destination(uint32_t ip) {
	if (!is_same_network_ip4(ip)) {
		if (current->gateway)
			return HashMap_get(Nodes, &current->gateway);
		else
			return NULL;
	}
	return HashMap_get(Nodes, &ip);
}

bool nodes_is_me(struct Node *node) {
	assert(node);
	if (node->ip4addr == current->ip4addr) {
		return true;
	}
	return false;
}

struct Node *nodes_get_gateway() {
	return nodes_get_node(current->gateway);
}

struct Node **nodes_get() {
	return (struct Node **)HashMap_getValues(Nodes, NULL);
}

struct Node *nodes_get_me() {
	return Me;
}

bool nodes_is_my_addr(uint32_t ip4) {
	return ip4 == current->ip4addr;
}
