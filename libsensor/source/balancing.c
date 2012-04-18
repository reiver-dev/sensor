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
#include "debug.h"
#include "util.h"
#include "nodes.h"


#include "services/info.h"

enum {
	STATE_BEGIN,
	STATE_WAIT_SENSORS,
	STATE_ALONE,
	STATE_COUPLE,
};

char *state_text[] = {
	"STATE_BEGIN",
	"STATE_WAIT_SENSORS",
	"STATE_ALONE",
	"STATE_COUPLE"
};


struct balancer {
	int udp_sock;
	time_t last_load;
	uint8_t State;
	struct CurrentAddress *current;
};


bool is_same_network_ip4(Balancer self, uint32_t ip) {
	uint32_t network = self->current->ip4addr & self->current->netmask;
	return (network & ip) == network;
}


void seek_sensors(Balancer self) {
	InfoRequest request;
	request.type = INFO_TYPE_POP;
	Services_Invoke(self->udp_sock, SERVICE_INFO, 0, &request);
}

void take_all_nodes(Balancer self) {
	int node_count = nodes_count();
	struct Node *nodes = nodes_get();

	for (int i = 0; i < node_count; i++) {
		if (nodes[i].is_online && nodes[i].ip4addr != self->current->gateway) {
			node_take(&nodes[i]);
		}
	}

}

int load_compare(const void *m1, const void *m2) {
	uint32_t first = (*(struct NodeLoad **)m1)->load;
	uint32_t second = (*(struct NodeLoad **)m2)->load;
	return first == second ? 0 : first > second ? 1 : -1;
}

struct NodeLoad *load_create_item(struct Node *client) {
	assert(client->type == NODE_TYPE_CLIENT);

	struct NodeLoad *newLoad = malloc(sizeof(*newLoad));
	memset(newLoad, 0, sizeof(*newLoad));
	newLoad->timestamp = time(NULL);

	ArrayList momentLoad = client->info.client.moment_load;
	ArrayList_add(momentLoad, newLoad);

	return newLoad;
}

struct NodeLoad *load_get_last(struct Node *client) {
	assert(client->type == NODE_TYPE_CLIENT);
	ArrayList momentLoad = client->info.client.moment_load;
	int loads = ArrayList_length(momentLoad);
	if (!loads) {
		return NULL;
	}
	return ArrayList_get(momentLoad, loads - 1);
}

void load_bytes_add(struct Node *client, int len) {
	struct NodeLoad *momentLoad = load_get_last(client);
	if (!momentLoad) {
		momentLoad = load_create_item(client);
	}
	momentLoad->load += len;
}


void load_close(struct Node *client, uint32_t interval) {

	ArrayList momentLoad = client->info.client.moment_load;
	int loads = ArrayList_length(momentLoad);

	void **data = ArrayList_getData(momentLoad);
	qsort(data, loads, sizeof(void *), load_compare);

	int load;
	if (loads % 2) {
		int index = loads / 2;
		struct NodeLoad *median = ArrayList_get(momentLoad, index);
		load = median->load;
	} else {
		int index = loads / 2;
		struct NodeLoad *median = ArrayList_get(momentLoad, index);
		load = median->load;
		index++;
		median = ArrayList_get(momentLoad, index);
		load += median->load;
		load /= 2;
	}

	load /= interval;

	DINFO("New load of node (%s) is %i\n", Ip4ToStr(client->ip4addr), load);

	client->info.client.load.load = load;
	client->info.client.load.timestamp = time(NULL);

}

struct Node *get_client(Balancer self, uint8_t *buffer, int length) {
	if (length < (sizeof(struct ether_header) + sizeof(struct iphdr))) {
		return NULL;
	}

	int position = sizeof(struct ether_header);
	struct ether_header *ethernet = (struct ether_header*) (buffer);

	uint16_t ethernetType = htons(ethernet->ether_type);
	if (ethernetType == ETH_P_IP) {
		struct iphdr *ipheader = (struct iphdr*) (buffer + position);
		struct Node *gw = node_get(self->current->gateway);
		struct Node *source	= node_get_destination(ipheader->saddr);
		if (source != NULL) {
			if (source == gw) {
				struct Node *dest = node_get_destination(ipheader->daddr);
				if (dest == NULL) {
					return NULL;
				} else {
					return dest;
				}
			} else {
				return source;
			}
		}
	}

	return NULL;
}

/* ---------------------------------------------- */
Balancer balancing_init(sensor_t config) {
	/* memorize current addreses */
	Balancer self = malloc(sizeof(*self));

	self->current = &config->current;

	self->udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(31337);
	bind(self->udp_sock, &sockaddr, sizeof(sockaddr));

	self->State = STATE_BEGIN;

	return self;

}

void balancing_destroy(Balancer self) {
	shutdown(self->udp_sock, 2);
	free(self);
}

#define IS_FILTER(x) if (x) return true
bool balancing_process_response(Balancer self, uint8_t *buffer, int length) {
	return false;
}
#undef IS_FILTER

bool balancing_add_load(Balancer self, uint8_t *buffer, int length) {
	struct Node *client = get_client(self, buffer, length);
	if (client != NULL) {
		if (client->type != NODE_TYPE_CLIENT) {
			DERROR("Can't count load, node is not client:\n%s\n", node_toString(client));
			client = get_client(self, buffer, length);
		} else{
			load_bytes_add(client, length);
		}
	}
	return false;
}

void balancing_count_load(Balancer self, uint32_t load_interval, uint32_t load_count) {
	ArrayList owned = nodes_get_owned();
	int len = ArrayList_length(owned);
	time_t now = time(NULL);

	for (int i = 0; i < len; i++) {
		struct Node *client = ArrayList_get(owned, i);
		struct NodeLoad *lastLoad = load_get_last(client);

		if (!lastLoad) {
			continue;
		}

		ArrayList momentLoads = client->info.client.moment_load;
		int loads = ArrayList_length(momentLoads);

		uint32_t interval = now - lastLoad->timestamp;
		if (interval >= load_interval && loads >= load_count) {
			load_close(client, load_interval);
			ArrayList_remove(momentLoads, 0);
			ArrayList_remove(momentLoads, ArrayList_length(momentLoads) - 1);
		} else if (interval >= load_interval) {
			load_create_item(client);
		}

	}
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
				self->State = STATE_ALONE;
			} else {
				self->State = STATE_COUPLE;
			}
			repeat = true;
			break;

		case STATE_ALONE:
			take_all_nodes(self);
			break;

		case STATE_COUPLE:
			DERROR("State not implemented yet: %s\n", state_text[STATE_COUPLE]);
			exit(1);
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




