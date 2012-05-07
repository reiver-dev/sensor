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
	} else if (ArrayList_isEmpty(sensor->info.sensor.clients)) {
		DNOTIFY("Sensor (%s) has 0 nodes\n", Ip4ToStr(sensor->ip4addr));
		result = false;
	} else {

		ArrayList clients = sensor->info.sensor.clients;
		result = ArrayList_contains(clients, client, 0);

	} /* if */

	return result;
}


bool unset_owned(struct Node *client) {
	if (!check_node_type(client, NODE_TYPE_CLIENT)) {
		return false;
	}

	struct Node *sensor = client->info.client.owned_by;
	if (!sensor) {
		return true;
	}

	client->info.client.owned_by = NULL;
	ArrayList ownedNodes = sensor->info.sensor.clients;
	int owned = ArrayList_indexOf(ownedNodes, client, 0);
	assert(owned >= 0);
	ArrayList_steal(ownedNodes, owned);

	return true;
}


bool set_owned_by(struct Node *sensor, struct Node *client) {
	if (!check_sensor_client(sensor, client)) {
		return false;
	}

	DINFO("Setting node ownership of sensor (%s) to ", Ip4ToStr(sensor->ip4addr));
	DINFOA("node (%s)\n", Ip4ToStr(client->ip4addr));
	if (client->info.client.owned_by != NULL) {
		if (client->info.client.owned_by == sensor) {
			return true;
		} else {
			unset_owned(client);
		}
	}

	ArrayList clients = sensor->info.sensor.clients;
	assert(clients);

	ArrayList_add(clients, client);
	sensor->info.sensor.clients = clients;
	client->info.client.owned_by = sensor;

	return true;
}



/* node destructors */
static void _client_init(struct Node *client) {
	DINFO("Node (%s) is now client\n", Ip4ToStr(client->ip4addr));
	memset(&client->info, 0, sizeof(client->info));
	client->info.client.moment_load = ArrayList_init(0, free);
	client->info.client.owned_by = NULL;
	client->type = NODE_TYPE_CLIENT;
}

static void _client_destroy(struct Node *client) {
	ArrayList_destroy(client->info.client.moment_load);
}

static void _sensor_init(struct Node *sensor) {
	DINFO("Node (%s) is now sensor\n", Ip4ToStr(sensor->ip4addr));
	memset(&sensor->info, 0, sizeof(sensor->info));
	sensor->info.sensor.clients = ArrayList_init(0, (ArrayList_destroyer)_client_destroy);
	sensor->type = NODE_TYPE_SENSOR;
}

static void _sensor_destroy(struct Node *sensor) {
	ArrayList_destroy(sensor->info.sensor.clients);
}

static void _gateway_init(struct Node *gw) {
	DINFO("Node (%s) is now gateway\n", Ip4ToStr(gw->ip4addr));
	gw->type = NODE_TYPE_GATEWAY;
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
		node->type    = NODE_TYPE_UNKNOWN;
	}

	Me = node_get(curr->ip4addr);
	_sensor_init(Me);
	SensorNodes = ArrayList_init(0, (ArrayList_destroyer) _sensor_destroy);

	if (curr->gateway) {
		struct Node *gw = node_get(curr->gateway);
		_gateway_init(gw);
	}

}

void nodes_destroy() {
	ArrayList_destroy(SensorNodes);
	_sensor_destroy(Me);
	free(Nodes);
	NodeCount = 0;
}



/* services */
void node_unset(struct Node *node) {
	int temp;

	switch (node->type) {
	case NODE_TYPE_CLIENT:
		unset_owned(node);
		_client_destroy(node);
		break;
	case NODE_TYPE_SENSOR:
		temp = ArrayList_indexOf(SensorNodes, node, 0);
		ArrayList_remove(SensorNodes, temp);
		break;
	}
}

void node_set_sensor(struct Node *node) {
	DINFO("Node (%s) is becoming sensor\n", Ip4ToStr(node->ip4addr));
	if (node->type != NODE_TYPE_SENSOR)
		node_unset(node);
	_sensor_init(node);
	ArrayList_add(SensorNodes, node);
}

void node_set_client(struct Node *node) {
	DINFO("Node (%s) is becoming client\n", Ip4ToStr(node->ip4addr));
	if (node->type != NODE_TYPE_CLIENT)
		node_unset(node);
	_client_init(node);
}

void node_set_gateway(struct Node *node) {
	DINFO("Node (%s) is becoming gateway\n", Ip4ToStr(node->ip4addr));
	if (node->type != NODE_TYPE_GATEWAY)
		node_unset(node);
	_gateway_init(node);
}

void node_set_owned_by(struct Node *sensor, uint32_t ip4addr, struct NodeLoad load) {
	struct Node *client = node_get(ip4addr);
	if (client == NULL) {
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4addr));
		DWARNINGA("received from sensor=(%s) not found\n", Ip4ToStr(sensor->ip4addr));
	} else if (node_is_me(client)) {
		DWARNING("Node conflict: (%s) is trying to take over ME\n", Ip4ToStr(sensor->ip4addr));
	} else if (is_owned_by(Me, client)) {
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4addr));
		DWARNINGA("is claimed by sensor=(%s) as his\n", Ip4ToStr(sensor->ip4addr));
	} else if (is_owned_by(sensor, client)) {
		client->info.client.load = load;
	} else {
		set_owned_by(sensor, client);
		client->info.client.load = load;
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

void node_take(struct Node *node) {
	assert(node);
	set_owned_by(Me, node);
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

int nodes_count() {
	return NodeCount;
}

struct Node *nodes_get() {
	return Nodes;
}

bool node_is_me(struct Node *node) {
	return node != NULL && node == Me;
}

ArrayList nodes_get_owned() {
	return Me->info.sensor.clients;
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
