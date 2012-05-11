#include <stdlib.h>
#include <stdio.h>
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
#include <netinet/ip.h>

#include <arpa/inet.h>

#include "balancing.h"
#include "load_count.h"
#include "debug.h"
#include "util.h"
#include "nodes.h"
#include "packet_extract.h"
#include "hashmap.h"


#include "services/info.h"
#include "services/bootstrap.h"
#include "services/node.h"
#include "bestfit.h"

char *state_text[] = {
	"STATE_BEGIN",
	"STATE_WAIT_SENSORS",
	"STATE_ALONE",
	"STATE_COUPLE"
};


struct SensorSession {
	struct Node *node;
	ArrayList owned;
};

struct balancer {
	time_t last_load;
	uint8_t State;
	struct CurrentAddress *current;
	ServicesData servicesData;
	HashMap sensorSessions;
	HashMap clientMomentLoads;
};



bool is_same_network_ip4(Balancer self, uint32_t ip) {
	uint32_t network = self->current->ip4addr & self->current->netmask;
	return (network & ip) == network;
}

/* -------------------------- loading */



static struct Node *get_client(Balancer self, uint8_t *buffer, int length) {
	struct iphdr *ipheader = packet_map_ip(buffer, length);
	if (!ipheader) {
		return NULL;
	}

	struct Node *gw = node_get_gateway();

	struct Node *source	= node_get_destination(ipheader->saddr);
	if (source && source != gw && source != node_get_me()) {
		return source;
	}

	struct Node *dest = node_get_destination(ipheader->daddr);
	if (dest && dest != gw) {
		return dest;
	}

	return NULL;

}

/* ---------------------------------------------- */

Balancer balancing_init(sensor_t config) {
	Balancer self = malloc(sizeof(*self));
	/* memorize current addreses */
	self->current = &config->current;
	self->State = STATE_BEGIN;
	self->servicesData = Services_Init(self, config->opt.device_name);

	self->sensorSessions = HashMap_initInt32(free, 0);
	self->clientMomentLoads = HashMap_initInt32(free, (HashMapDestroyer)ArrayList_destroy);

	return self;
}

void balancing_destroy(Balancer self) {
	Services_Destroy(self->servicesData);
	free(self);
}

#define IS_FILTER(x) if (x) return true
bool balancing_filter_response(Balancer self, uint8_t *buffer, int length) {
	IS_FILTER(Services_isResponse(buffer, length));
	return false;
}
#undef IS_FILTER

void balancing_add_load(Balancer self, uint8_t *buffer, int length) {
	struct Node *client = get_client(self, buffer, length);
	if (client != NULL) {
		if (client->type == NODE_TYPE_CLIENT) {
			ArrayList momentLoads = HashMap_get(self->clientMomentLoads, &client->ip4addr);
			load_bytes_add(momentLoads, length);
		} else {
//			struct iphdr *ip_header = packet_map_ip(buffer, length);
//			DERROR("Can't count load, (%s - ", Ip4ToStr(ip_header->saddr));
//			DERRORA("%s) ", Ip4ToStr(ip_header->daddr));
//			DERRORA("node is:\n%s\n", node_toString(client));
		}
	}
}

void balancing_count_load(Balancer self, uint32_t l_interval, uint32_t l_count) {
	load_count(self->clientMomentLoads, l_interval, l_count);
}

/* -------------------------------- State Machine */

ArrayList balancing_get_owned(Balancer self) {
	uint32_t **addreses = (uint32_t **)HashMap_getKeys(self->clientMomentLoads);

	if (!addreses) {
		return NULL;
	}
	size_t count = HashMap_size(self->clientMomentLoads);

	ArrayList owned = ArrayList_init(count, 0);

	while(*addreses) {
		ArrayList_add(owned, node_get(**addreses));
		addreses++;
	}

	return owned;
}

void balancing_receive_service(Balancer self) {
	Services_Receive(self->servicesData);
}

uint8_t balancing_get_state(Balancer self) {
	return self->State;
}

void seek_sensors(Balancer self) {
	BootstrapRequest request = {BOOTSTRAP_TYPE_CONNECT};
	Services_Request(self->servicesData, BootstrapService_Get(), 0, &request);
}

void release_node(Balancer self, struct Node *client) {

}

void balancing_take_node(Balancer self, uint32_t ip4addr) {
	struct Node *client = node_get(ip4addr);
	if (client->is_online && client->ip4addr != self->current->gateway) {
		uint32_t ip4addr = client->ip4addr;
		if (!HashMap_contains(self->clientMomentLoads, &ip4addr)) {
			client->owned_by = node_get_me();
			ArrayList loads = ArrayList_init(0, free);
			HashMap_addInt32(self->clientMomentLoads, ip4addr, loads);
		}
	}
}

void take_sensor_client(struct SensorSession *session, struct Node *client) {
	if (client->owned_by == session->node) {
		return;
	} else {
		client->owned_by = session->node;
		ArrayList_add(session->owned, client);
	}
}

void take_all_nodes(Balancer self) {
	int node_count = nodes_count();
	struct Node *nodes = nodes_get();

	for (int i = 0; i < node_count; i++) {
		balancing_take_node(self, nodes[i].ip4addr);
	}
}

void balancing_node_owned(Balancer self, uint32_t ip4s, uint32_t ip4c) {
	struct Node *sensor = node_get(ip4s);
	struct Node *client = node_get(ip4c);

	if (client == NULL) {
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4c));
		DWARNINGA("received from sensor=(%s) not found\n", Ip4ToStr(sensor->ip4addr));

	} else if (client == node_get_me()) {
		DWARNING("Node conflict: (%s) is trying to take over ME\n", Ip4ToStr(sensor->ip4addr));

	} else if (client->owned_by == node_get_me()) {
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4c));
		DWARNINGA("is claimed by sensor=(%s) as his\n", Ip4ToStr(sensor->ip4addr));

	} else {
		struct SensorSession *session = HashMap_get(self->sensorSessions, &ip4s);
		if (session == NULL) {
			DERROR("Sensor session not exist (%s)\n", Ip4ToStr(ip4s));
		} else {
			take_sensor_client(session, client);
		}
	}
}













int client_load_compare_dec(const void *c1, const void *c2) {
	int load1 = (*(struct Node **) c1)->load;
	int load2 = (*(struct Node **) c2)->load;

	return load1 == load2 ? 0 : load1 < load2 ? 1 : -1;
}

int clients_load_sum(struct Node **clients, int client_count) {
	int result = 0;
	for (int i = 0; i < client_count; i++) {
		result += clients[i]->load;
	}
	return result;
}


static int sort_node_by_ip(const void *n1, const void *n2) {
	uint32_t ip1 = (*(struct Node **)n1)->ip4addr;
	uint32_t ip2 = (*(struct Node **)n2)->ip4addr;
	return ip1 == ip2 ? 0 : ip1 < ip2 ? -1 : 1;
}

ArrayList count_nodes_to_take(ArrayList sensors) {
	ArrayList sensorsCopy = ArrayList_copy(sensors);

	ArrayList_add(sensorsCopy, node_get_me());
	ArrayList_qsort(sensorsCopy, sort_node_by_ip);

	ArrayList solution = bestfit_solution(sensorsCopy);

	size_t me_index = ArrayList_indexOf(sensorsCopy, node_get_me(), NULL);
	ArrayList clients = ArrayList_get(solution, me_index);
	ArrayList clientsCopy = ArrayList_copy(clients);

	ArrayList_destroy(solution);
	ArrayList_destroy(sensorsCopy);

	return clientsCopy;
}

void rebalance_nodes(Balancer self) {
	ArrayList sensors = nodes_get_sensors();
	ArrayList nodes = count_nodes_to_take(sensors);
	size_t count = ArrayList_length(nodes);
	DNOTIFY("Taking (%i) nodes\n", count);
	for (int i = 0; i < count; i++) {
		struct Node *client = ArrayList_get(nodes, i);
		if (!node_is_me(client->owned_by)) {
			uint32_t ip4addr = client->ip4addr;
			DNOTIFYA("%s\n", Ip4ToStr(ip4addr));
			Array array = Array_init(0, sizeof(ip4addr));
			Array_add(array, &ip4addr);
			NodeRequest request = {
				.type = NODESERVICE_TYPE_TAKE,
				.ip4array = array
			};
			Services_Request(self->servicesData, NodeService_Get(), client->owned_by, &request);
			Array_destroy(array);
		}
	}
	ArrayList_destroy(nodes);
}

void to_STATE_ALONE(Balancer self) {
	self->State = STATE_ALONE;
	take_all_nodes(self);
}

void to_STATE_COUPLE(Balancer self) {
	self->State = STATE_COUPLE;
	rebalance_nodes(self);
}

void balancing_process(Balancer self) {
	DINFO("%s\n", "Starting balancing");

	bool repeat ;
	int State = self->State;

	if (self->State <  sizeof(state_text)) {
		DINFO("Current State = %s\n", state_text[self->State]);
	}

	do {
		repeat = false;

		switch(self->State) {
		case STATE_BEGIN:
			seek_sensors(self);
			self->State = STATE_WAIT_SENSORS;
			break;

		case STATE_WAIT_SENSORS:
			if (!ArrayList_length(nodes_get_sensors())) {
				to_STATE_ALONE(self);
			} else {
				to_STATE_COUPLE(self);
			}
			repeat = true;
			break;

		case STATE_ALONE:
			if (!ArrayList_length(nodes_get_sensors())) {
				take_all_nodes(self);
			} else {
				to_STATE_COUPLE(self);
			}
			break;

		case STATE_COUPLE:
			if (!ArrayList_length(nodes_get_sensors())) {
				to_STATE_ALONE(self);
			} else {
				rebalance_nodes(self);
			}
			break;

		default:
			DERROR("State is not specified: %i\n", self->State);
			exit(1);
		}

	} while (repeat);

	if (State != self->State) {
		DINFO("New State = %s\n", state_text[self->State]);
	}

	DINFO("%s\n", "Balancing finished");
}




