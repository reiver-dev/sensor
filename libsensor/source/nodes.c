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

static struct Node *Me;
static ArrayList SensorNodes;

static struct CurrentAddress *current;


static char *get_node_type(int type) {
	switch (type) {
	case NODE_TYPE_CLIENT:
		return "client";
		break;
	case NODE_TYPE_SENSOR:
		return "sensor";
		break;
	case NODE_TYPE_GATEWAY:
		return "gateway";
		break;
	case NODE_TYPE_UNKNOWN:
		return "unknown";
	default:
		return "UNDEFINED_TYPE";
	}
}


static bool check_node_type(struct Node *node, int expectedType) {
	assert(node);
	if (node->type == expectedType) {
		return true;
	} else {
		DWARNING("Node (%s) is %s - expected %s\n",
				Ip4ToStr(node->ip4addr),
				get_node_type(node->type),
				get_node_type(expectedType));
		return false;
	}
}

static bool check_sensor_client(struct Node *sensor, struct Node *client) {
	return check_node_type(sensor, NODE_TYPE_SENSOR) && check_node_type(client, NODE_TYPE_CLIENT);
}

static uint32_t get_node_index(uint32_t ip) {
	uint32_t ind = ntohl(ip) - ntohl(current->ip4addr & current->netmask) - 1;
	return ind;
}


/* node conditions*/
bool is_owned_by(struct Node *sensor, struct Node *client) {

	bool result;
	if (!check_sensor_client(sensor, client)) {
		result = false;
	} else if (client->owned_by == sensor) {
		result = true;
	} /* if */

	return result;
}


/* node destructors */
static void _node_init(struct Node *client) {
	client->owned_by = NULL;
	client->load = 0;
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
		node->ip4addr = htonl(network + i + 1);
		memset(node->hwaddr, 0, ETH_ALEN);
		node->type    = NODE_TYPE_UNKNOWN;
		node->last_check = 0;
		node->is_online = false;
	}

	Me = node_get(curr->ip4addr);
	node_set_sensor(Me);
	SensorNodes = ArrayList_init(0, (ArrayList_destroyer) node_set_client);

	if (curr->gateway) {
		struct Node *gw = node_get(curr->gateway);
		node_set_gateway(gw);
	}

}

void nodes_destroy() {
	ArrayList_destroy(SensorNodes);
	free(Nodes);
	NodeCount = 0;
}


void node_set_sensor(struct Node *node) {
	DINFO("Node (%s) is becoming sensor\n", Ip4ToStr(node->ip4addr));
	if (node->type != NODE_TYPE_SENSOR) {
		_node_init(node);
		node->type = NODE_TYPE_SENSOR;
	}
}

void node_set_client(struct Node *node) {
	DINFO("Node (%s) is becoming client\n", Ip4ToStr(node->ip4addr));
	if (node->type != NODE_TYPE_CLIENT) {
		_node_init(node);
		node->type = NODE_TYPE_CLIENT;
	}
}

void node_set_gateway(struct Node *node) {
	DINFO("Node (%s) is becoming gateway\n", Ip4ToStr(node->ip4addr));
	if (node->type != NODE_TYPE_GATEWAY) {
		_node_init(node);
		node->type = NODE_TYPE_GATEWAY;
	}
}

void node_answered(uint32_t ip4, uint8_t *hw) {

	struct Node *node = node_get(ip4);

	DINFO("Node last check was: %i\n", node->last_check);

	if (node->is_online) {
		DINFO("Node (%s) is still online\n", Ip4ToStr(node->ip4addr));
	} else {
		if (node->type == NODE_TYPE_UNKNOWN) {
			node_set_client(node);
		}
		memcpy(node->hwaddr, hw, ETH_ALEN);
		node->is_online = true;
	}

	node->last_check = time(0);
}

struct Node *node_get(uint32_t ip) {
	uint32_t index = get_node_index(ip);
	if (index < 0 || index > NodeCount) {
		DERROR("Node with address %s not found\n", Ip4ToStr(ip));
		return NULL;
	}
	return &Nodes[index];
}

struct Node *node_get_destination(uint32_t ip) {
	uint32_t index = get_node_index(ip);
	if (index < 0 || index > NodeCount) {
		if (current->gateway)
			index = get_node_index(current->gateway);
		else
			return NULL;
	}
	return &Nodes[index];
}

struct Node *node_get_gateway() {
	if (current->gateway) {
		return node_get(current->gateway);
	}
	return NULL;
}


struct Node *node_get_me() {
	return Me;
}

int nodes_count() {
	return NodeCount;
}

struct Node *nodes_get() {
	return Nodes;
}

bool node_is_me(struct Node *node) {
	return node != NULL && node == Me;
}

ArrayList nodes_get_sensors() {
	return SensorNodes;
}

#define STRVAL(str, key, val) sprintf(str + strlen(str), "%s = %s\n", key, val);
char *node_toString(struct Node *node) {
	static char str[128];
	memset(str, 0, 128);

	char *node_type;
	switch (node->type) {
	case NODE_TYPE_CLIENT:
		node_type = "NODE_TYPE_CLIENT";
		break;
	case NODE_TYPE_SENSOR:
		node_type = "NODE_TYPE_SENSOR";
		break;
	case NODE_TYPE_GATEWAY:
		node_type = "NODE_TYPE_GATEWAY";
		break;
	default:
		node_type = "NODE_TYPE_UNKNOWN";
		break;
	}

	char last_check[32];
	strftime(last_check, sizeof(last_check), "%T", localtime(&node->last_check));

	STRVAL(str, "IP", Ip4ToStr(node->ip4addr));
	STRVAL(str, "HW", EtherToStr(node->hwaddr));
	STRVAL(str, "Online", node->is_online ? "true" : "false");
	STRVAL(str, "Type", node_type);
	STRVAL(str, "Last Check", last_check);

	return str;
}
