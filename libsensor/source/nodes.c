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

/* Locals */

static struct Node *Nodes;
static unsigned long NodeCount;


#define MAXOWNED 255
#define MAXSENSORS 255
static struct Node **OwnedNodes;
static struct Node **SensorNodes;
static int SensorCount;
static int OwnedCount;


static struct CurrentAddress *current;



/* private utils */
static uint32_t get_node_index(uint32_t ip) {
	uint32_t ind = ntohl(ip) - ntohl(current->ip4addr & current->netmask) - 1;
	return ntohl(ip) < ntohl(current->ip4addr) ? ind : ind - 1;
}


static int get_owned_index(struct Node *node) {
	int i;
	for (i = 0; i < OwnedCount; i ++) {
		if (OwnedNodes[i] == node) {
			return i;
		}
	}
	return -1;
}


/* node conditions*/
bool is_owned_by_me(struct Node *client) {
	assert(client);
	bool result = false;

	if (client->type != NODE_CLIENT) {
		DWARNING("Node %s is not client", Ip4ToStr(client->ip4addr));
	} else if (OwnedCount == 0) {
		DNOTIFY("I (%s) have 0 nodes", Ip4ToStr(current->ip4addr));
	} else {

		for (int i = 0; i < OwnedCount; i++) {
			struct Node *node = OwnedNodes[i];
			if (node == client) {
				result = true;
				break;
			}
		}

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
	} else if (sensor->info.sensor.clients_count == 0) {
		DNOTIFY("Sensor %s has 0 nodes", Ip4ToStr(sensor->ip4addr));
	} else {

		for (int i = 0; i < sensor->info.sensor.clients_count; i++) {
			struct Node *node = sensor->info.sensor.clients[i];
			if (node == client) {
				result = true;
				break;
			}
		}

	} /* if */

	return result;
}


int take_ownership(struct Node *node) {
	assert(node);         /* in array bounds   */
	assert(node->is_online);

	if (node->type == NODE_SENSOR) {
		DWARNING("Tried to take ownership over sensor: IP4 = %s", Ip4ToStr(node->ip4addr));
		return 1;
	} else if (OwnedCount == MAXOWNED - 1) {
		DWARNING("%s", "Max nodes reached");
		return 1;
	}

	node->info.client.type = CLIENT_OWNED;

	OwnedNodes[OwnedCount] = node;
	OwnedCount++;

	return 0;
}


void add_owned_to(struct Node *sensor, struct Node *client) {
	assert(sensor);
	assert(client);

	int count = sensor->info.sensor.clients_count;
	struct Node **clients;

	if (count == 0) {
		clients = NULL;
	} else {
		clients = sensor->info.sensor.clients;
	}

	clients = Reallocate(clients, sizeof(void **), count, count + 1, 10);
	clients[count] = client;

	sensor->info.sensor.clients_count++;
	clients = sensor->info.sensor.clients = clients;

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


/* Main functions */
void nodes_init(struct CurrentAddress *curr) {
	/* memorize current addreses */

	current = curr;

	NodeCount = (1 << (32 - bitcount(current->netmask))) - 2;
	Nodes = malloc(NodeCount * sizeof(*Nodes));
	memset(Nodes, '\0', NodeCount);

	uint32_t network = ntohl(current->ip4addr & current->netmask);
	DNOTIFY("Node count is: %i\n", NodeCount);
	uint32_t tmp;
	uint32_t curr_ip = ntohl(current->ip4addr);
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

void nodes_destroy() {

	for (int i = 0; i < SensorCount; i++) {
		if (SensorNodes[i]->info.sensor.clients != NULL) {
			free(SensorNodes[i]->info.sensor.clients);
		}
	}

	free(SensorNodes);
	free(OwnedNodes);
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

struct Node **nodes_get_owned() {
	return OwnedNodes;
}

int nodes_owned_count() {
	return OwnedCount;
}

struct Node **nodes_get_sensors() {
	return SensorNodes;
}

int nodes_sensor_count() {
	return SensorCount;
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
