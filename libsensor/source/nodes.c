#include <assert.h>
#include <stdlib.h>
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
static unsigned long NodeCount;


#define MAXOWNED 255
#define MAXSENSORS 255

static ArrayList OwnedNodes;
static ArrayList SensorNodes;

static struct CurrentAddress *current;



/* private utils */
static bool by_ip4(struct Node *a, struct Node *b) {
	return a->ip4addr == b->ip4addr;
}


static uint32_t get_node_index(uint32_t ip) {
	uint32_t ind = ntohl(ip) - ntohl(current->ip4addr & current->netmask) - 1;
	return ntohl(ip) < ntohl(current->ip4addr) ? ind : ind - 1;
}


/* node conditions*/
bool is_owned_by_me(struct Node *client) {
	assert(client);
	bool result = false;

	if (client->type != NODE_CLIENT) {
		DWARNING("Node %s is not client", Ip4ToStr(client->ip4addr));
	} else if (ArrayList_length(OwnedNodes) == 0) {
		DNOTIFY("I (%s) have 0 nodes", Ip4ToStr(current->ip4addr));
	} else {

		result = ArrayList_contains(OwnedNodes, client, 0);

	} /* if */

	return result;
}

bool is_owned_by(struct Node *sensor, struct Node *client) {
	assert(sensor);
	assert(client);

	bool result = false;
	if (sensor->type != NODE_SENSOR) {
		DWARNING("Node %s is not sensor", Ip4ToStr(sensor->ip4addr));
	} else if (client->type != NODE_CLIENT) {
		DWARNING("Node %s is not client", Ip4ToStr(client->ip4addr));
	} else if (ArrayList_isEmpty(sensor->info.sensor.clients)) {
		DNOTIFY("Sensor %s has 0 nodes", Ip4ToStr(sensor->ip4addr));
	} else {

		ArrayList clients = sensor->info.sensor.clients;
		result = ArrayList_contains(clients, client, 0);

	} /* if */

	return result;
}


int take_ownership(struct Node *node) {
	assert(node);         /* in array bounds   */
	assert(node->is_online);

	if (node->type == NODE_SENSOR) {
		DWARNING("Tried to take ownership over sensor: IP4 = %s", Ip4ToStr(node->ip4addr));
		return 1;
	} else if (ArrayList_length(OwnedNodes) == MAXOWNED - 1) {
		DWARNING("%s", "Max nodes reached");
		return 1;
	}

	node->info.client.type = CLIENT_OWNED;

	ArrayList_add(OwnedNodes, node);

	return 0;
}


void add_owned_to(struct Node *sensor, struct Node *client) {
	assert(sensor);
	assert(client);

	ArrayList clients;

	if (sensor->info.sensor.clients == NULL) {
		clients = ArrayList_init(0, 0);
	} else {
		clients = sensor->info.sensor.clients;
	}

	ArrayList_add(clients, client);
	sensor->info.sensor.clients = clients;

}

int release_ownership(struct Node *client) {
	if (client->type != NODE_CLIENT && client->info.client.type != CLIENT_OWNED) {
		DWARNING("Tried to release not owned node: IP4 = %s", Ip4ToStr(client->ip4addr));
		return 1;
	}

	int owned = ArrayList_indexOf(OwnedNodes, client, 0);
	assert(owned);

	client->info.client.type = CLIENT_FREE;
	ArrayList_remove(OwnedNodes, owned);


	return 0;
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
	uint32_t tmp;
	uint32_t curr_ip = ntohl(current->ip4addr);
	for (uint32_t i = 0; i < NodeCount; i++) {
		struct Node *node = & Nodes[i];
		tmp = network + i + 1;
		node->ip4addr = htonl(tmp < curr_ip ? tmp : tmp + 1);
		node->type    = NODE_UNKNOWN;
	}

	OwnedNodes = ArrayList_init(0, 0, 0);
	SensorNodes = ArrayList_init(0, 0, 0);

}

void nodes_destroy() {

	ArrayList_destroy(SensorNodes);
	ArrayList_destroy(OwnedNodes);
	free(Nodes);

	NodeCount = 0;
}


int nodes_count() {
	return NodeCount;
}

struct Node *nodes_get() {
	return Nodes;
}

struct Node *node_get(uint32_t ip) {
	uint32_t index = get_node_index(ip);
	if (index < 0 || index > NodeCount) {
		DERROR("Node with address %s not found", Ip4ToStr(ip));
		return NULL;
	}
	return &Nodes[index];
}

ArrayList nodes_get_owned() {
	return OwnedNodes;
}

ArrayList nodes_get_sensors() {
	return SensorNodes;
}

/* services */
void node_answered(uint32_t ip4, uint8_t *hw) {

	struct Node *node = node_get(ip4);

	DINFO("Node last check was: %i\n", node->last_check);

	if (node->is_online) {
		DINFO("Node IP:%s is still online\n", Ip4ToStr(node->ip4addr));
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

void node_set_owned_by(struct Node *sensor, uint32_t ip4addr, int load) {
	struct Node *client = node_get(ip4addr);

	if (client == NULL) {
		DWARNING("Node received from %s not found", Ip4ToStr(sensor->ip4addr));
	} else if (is_owned_by_me(client)) {
		DWARNING("Node conflict: sensor %s claims node as his", Ip4ToStr(sensor->ip4addr));
		DWARNING("Node conflict: conflicted node is %s", Ip4ToStr(client->ip4addr));
	} else if (is_owned_by(sensor, client)) {
		client->info.client.load = load;
	} else {
		add_owned_to(sensor, client);
		client->info.client.load = load;
	}

}

void node_take(struct Node *node) {
	assert(node);
	take_ownership(node);
}
