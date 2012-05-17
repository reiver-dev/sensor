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
#include "arraylist.h"


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
	time_t created;
	time_t last_info;
	struct Node *node;
	ArrayList owned;
};

struct balancer {

	time_t info_interval;
	time_t session_interval;
	time_t survey_interval;

	uint8_t State;

	struct CurrentAddress *current;

	struct SensorSession Me;
	HashMap clientMomentLoads;

	ServicesData servicesData;

	HashMap sensorSessions;

};

void debug_client(void *n) {
	struct Node *node = n;
	DINFOA("Node: (%s) -- (%i)\n", Ip4ToStr(node->ip4addr), node->load);
}

void debug_owned_clients(Balancer self) {
	ArrayList owned = self->Me.owned;
	DINFO("%s", "Owned Nodes:\n");
	ArrayList_foreach(owned, debug_client);
}

bool is_same_network_ip4(Balancer self, uint32_t ip) {
	uint32_t network = self->current->ip4addr & self->current->netmask;
	return (network & ip) == network;
}

bool is_valid_source(Balancer self, uint32_t ip) {
	return  is_same_network_ip4(self, ip)
			&& ip != self->current->gateway
			&& ip != self->current->ip4addr;
}

bool balancing_is_in_session(Balancer self, uint32_t ip) {
	return HashMap_contains(self->sensorSessions, &ip);
}

bool balancing_is_valid_addreses(Balancer self, uint32_t ip4from, uint32_t ip4to) {
	if (ip4from == ip4to) {
		return false;
	}

	if (!is_same_network_ip4(self, ip4from) || !is_same_network_ip4(self, ip4to)) {
		return false;
	}

	if (ip4from == self->current->ip4addr && balancing_is_in_session(self, ip4to)) {
		return true;
	}

	if (ip4to == self->current->ip4addr && balancing_is_in_session(self, ip4from)) {
		return true;
	}

	return false;
}


/* callback when session removed hashmap */
static void session_destroy(struct SensorSession *session) {
	ArrayList_destroy(session->owned);
	free(session);
}

/* callback when node removed from owned */
static void unset_owned(struct Node *node) {
	node->owned_by = NULL;
}


void balancing_break_sensor_session(Balancer self, uint32_t ip4) {
	HashMap_remove(self->sensorSessions, &ip4);
}

void balancing_init_sensor_session(Balancer self, uint32_t ip4, time_t created) {
	if (!balancing_is_in_session(self, ip4)) {
		DNOTIFY("Creating session for sensor (%s)\n", Ip4ToStr(ip4));
		struct SensorSession *session = malloc(sizeof(struct SensorSession));
		session->node = nodes_get_node(ip4);
		session->owned = ArrayList_init(0, (ArrayList_destroyer)unset_owned);
		session->created = created;
		session->last_info = time(NULL);
		HashMap_addInt32(self->sensorSessions, ip4, session);
		balancing_release_node(self, ip4);
	}
}

Balancer balancing_init(sensor_t config) {
	Balancer self = malloc(sizeof(*self));
	/* memorize current addreses */
	self->current = &config->current;
	self->State = STATE_BEGIN;
	self->servicesData = Services_Init(self, config->opt.device_name);

	self->info_interval = config->opt.balancing.info_timeout;
	self->survey_interval = config->opt.survey.node_disconnect_timeout;
	self->session_interval = config->opt.balancing.session_timeout;

	self->sensorSessions = HashMap_initInt32(free, (HashMapDestroyer)session_destroy);
	self->clientMomentLoads = HashMap_initInt32(free, (HashMapDestroyer)ArrayList_destroy);

	self->Me.owned = ArrayList_init(0, (ArrayList_destroyer)unset_owned);
	self->Me.node = nodes_get_me();
	self->Me.created = time(NULL);

	return self;
}

void balancing_destroy(Balancer self) {
	Services_Destroy(self->servicesData);
	HashMap_destroy(self->clientMomentLoads);
	HashMap_destroy(self->sensorSessions);
	ArrayList_destroy(self->Me.owned);
	free(self);
}

#define IS_FILTER(x) if (x) return true
bool balancing_filter_response(Balancer self, uint8_t *buffer, int length) {
	IS_FILTER(Services_isResponse(buffer, length));
	return false;
}
#undef IS_FILTER

/* when shutdown */
void balancing_disconnect(Balancer self) {
	BootstrapRequest request = {BOOTSTRAP_TYPE_DISCONNECT};
	Services_Request(self->servicesData, BootstrapService_Get(), 0, &request);
}

void balancing_add_load(Balancer self, uint8_t *buffer, size_t length) {
	struct iphdr *ipheader = packet_map_ip(buffer, length);
	if (!ipheader)
		return;

	ArrayList momentLoads;

	momentLoads = HashMap_get(self->clientMomentLoads, &ipheader->saddr);
	if (momentLoads)
		load_bytes_add(momentLoads, length);

	momentLoads = HashMap_get(self->clientMomentLoads, &ipheader->daddr);
	if (momentLoads)
		load_bytes_add(momentLoads, length);

}

void balancing_count_load(Balancer self, uint32_t l_interval, uint32_t l_count) {
	load_count(self->clientMomentLoads, self->Me.owned, l_interval, l_count);
}

void balancing_receive_service(Balancer self) {
	Services_Receive(self->servicesData);
}

static void seek_sensors(Balancer self) {
	BootstrapRequest request = {BOOTSTRAP_TYPE_CONNECT};
	Services_Request(self->servicesData, BootstrapService_Get(), 0, &request);
}

void balancing_release_node(Balancer self, uint32_t ip4c) {
	struct Node *client = nodes_get_node(ip4c);
	if (client && client->owned_by && nodes_is_me(client->owned_by)) {
		DNOTIFY("Releasing node (%s)\n", Ip4ToStr(client->ip4addr));
		ArrayList_removeItem(self->Me.owned, client, NULL);
		HashMap_remove(self->clientMomentLoads, &client->ip4addr);
	}
}

static void take_node(Balancer self, uint32_t ip4addr) {
	if (balancing_is_in_session(self, ip4addr) || ip4addr == self->current->gateway) {
		return;
	}
	struct Node *client = nodes_get_node(ip4addr);
	if (client && client->is_online) {
		if (!HashMap_contains(self->clientMomentLoads, &ip4addr)) {
			DNOTIFY("Taking node (%s)\n", Ip4ToStr(ip4addr));
			client->owned_by = nodes_get_me();
			ArrayList loads = ArrayList_init(0, free);
			HashMap_addInt32(self->clientMomentLoads, ip4addr, loads);
			ArrayList_add(self->Me.owned, client);
		}
	}
}

void balancing_take_node_from(Balancer self, uint32_t ip4s, uint32_t ip4c) {
	struct SensorSession *session = HashMap_get(self->sensorSessions, &ip4s);
	struct Node *client = nodes_get_node(ip4c);
	assert(session);
	if (client == NULL) {
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4c));
		DWARNINGA("given by sensor=(%s) not found\n", Ip4ToStr(ip4s));
	} else if (client->owned_by && nodes_is_me(client->owned_by)) {
		DNOTIFY("Node collision resolved: node (%s) is mine\n",  Ip4ToStr(ip4c));
	} else if (client->owned_by != session->node) {
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4c));
		DWARNINGA("given by sensor=(%s) not owned by it\n", Ip4ToStr(ip4s));
	} else {
		ArrayList_removeItem(session->owned, client, NULL);
		take_node(self, ip4c);
	}

}

static void take_sensor_client(Balancer self, struct SensorSession *session, struct Node *client) {
	if (client->owned_by && client->owned_by == session->node) {
		return;
	} else if (client->owned_by && client->owned_by != session->node) {
		struct SensorSession *owner_session = HashMap_get(self->sensorSessions, &client->owned_by->ip4addr);
		assert(owner_session);
		ArrayList_removeItem(owner_session->owned, client, NULL);
	}
	client->owned_by = session->node;
	ArrayList_add(session->owned, client);
}

static void take_all_nodes(Balancer self) {
	int node_count = nodes_count();
	struct Node **nodes = nodes_get();

	for (int i = 0; i < node_count; i++) {
		take_node(self, nodes[i]->ip4addr);
	}
	free(nodes);
}

void balancing_sensor_info_refreshed(Balancer self, uint32_t ip4s) {
	struct SensorSession *session = HashMap_get(self->sensorSessions, &ip4s);
	assert(session);
	session->last_info = time(NULL);
}

void balancing_node_owned(Balancer self, uint32_t ip4s, uint32_t ip4c, uint32_t load) {
	struct SensorSession *session = HashMap_get(self->sensorSessions, &ip4s);
	struct Node *client = nodes_get_node(ip4c);
	if (session == NULL) { //  no session
		DERROR("Session for sensor (%s) not found\n",  Ip4ToStr(ip4s));
	} else if (ip4c == self->current->ip4addr) {
		DWARNING("Node conflict: (%s) is trying to take over ME\n", Ip4ToStr(ip4s));
	} else if (client == NULL) { // no client
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4c));
		DWARNINGA("received from sensor=(%s) not found\n", Ip4ToStr(ip4s));
	} else if (client->owned_by && client->owned_by == nodes_get_me()) { // client is taken by me
		DWARNING("Node conflict: node=(%s) ", Ip4ToStr(ip4c));
		DWARNINGA("is claimed by sensor=(%s) as his\n", Ip4ToStr(ip4s));

		balancing_release_node(self, client->ip4addr);
		Array array = Array_init(0, sizeof(ip4c));
		Array_add(array, &ip4c);
		NodeRequest request = {
			.type = NODESERVICE_TYPE_GIVE,
			.ip4array = array
		};
		Services_Request(self->servicesData, NodeService_Get(), session->node, &request);
		Array_destroy(array);
		take_sensor_client(self, session, client);
		client->load = load;

	} else {
		take_sensor_client(self, session, client);
		client->load = load;
	}


}


static int sort_sessions_by_time(const void *n1, const void *n2) {
	time_t t1 = (*(struct SensorSession **)n1)->created;
	time_t t2 = (*(struct SensorSession **)n2)->created;
	if (t1 == t1) {
		uint32_t ip1 = (*(struct SensorSession **) n1)->node->ip4addr;
		uint32_t ip2 = (*(struct SensorSession **) n2)->node->ip4addr;
		return ip1 == ip2 ? 0 : ip1 < ip2 ? -1 : 1;
	}

	return t1 < t2 ? -1 : 1;
}

static size_t getMyPosition (Balancer self) {
	size_t sessions_count = HashMap_size(self->sensorSessions) + 1;
	struct SensorSession **sessions = malloc(sessions_count * sizeof(*sessions));
	HashMap_getValues(self->sensorSessions, (void **)sessions);
	sessions[sessions_count - 1] = &self->Me;
	qsort(sessions, sessions_count, sizeof(*sessions), sort_sessions_by_time);

	size_t my_position = -1;
	for (size_t i = 0; i < sessions_count; i++) {
		DINFOA("S%i(%s)\t\t", i, Ip4ToStr(sessions[i]->node->ip4addr));
		if (sessions[i] == &self->Me) {
			my_position = i;
		}
	}
	DINFOA("%s", "\n");
	free(sessions);

	return my_position;
}

static ArrayList get_all_clients(Balancer self) {
	struct Node **nodes = nodes_get();
	size_t nlen = nodes_count();
	ArrayList clients = ArrayList_init(0, NULL);
	for (size_t i = 0; i < nlen; i++) {
		if (!balancing_is_in_session(self, nodes[i]->ip4addr)) {
			ArrayList_add(clients, nodes[i]);
		}
	}
	free(nodes);
	return clients;
}

static ArrayList count_nodes_to_take(Balancer self) {
	size_t my_position = getMyPosition(self);
	size_t sessions_count = HashMap_size(self->sensorSessions) + 1;

	ArrayList clients = get_all_clients(self);

	struct Node **clientsArr = (struct Node **) ArrayList_getData(clients);
	size_t clients_count = ArrayList_length(clients);
	ArrayList solution = bestfit_solution(clientsArr, clients_count, sessions_count);

#ifdef DEBUG

	size_t max_size = 0;
	size_t len = ArrayList_length(solution);
	for (size_t i = 0; i < len; i++) {
		ArrayList clients = ArrayList_get(solution, i);
		if (max_size < ArrayList_length(clients)) {
			max_size = ArrayList_length(clients);
		}
	}

	for (size_t i = 0; i < max_size; i++) {
		for (size_t j = 0; j < len; j++) {
			ArrayList clients = ArrayList_get(solution, j);
			if (i < ArrayList_length(clients)) {
				struct Node *node = ArrayList_get(clients, i);
				DINFOA("%s(%i)\t\t", Ip4ToStr(node->ip4addr), node->load);
			} else {
				DINFOA("%s\t\t", "              ");
			}
		}
		DINFOA("%s", "\n");
	}

#endif

	ArrayList my_clients = ArrayList_get(solution, my_position);
	ArrayList clientsCopy = ArrayList_copy(my_clients);

	ArrayList_destroy(clients);
	ArrayList_destroy(solution);

	return clientsCopy;
}

static void rebalance_nodes(Balancer self) {
	ArrayList nodes = count_nodes_to_take(self);
	size_t count = ArrayList_length(nodes);
	DNOTIFY("Taking (%i) nodes\n", count);
	for (int i = 0; i < count; i++) {
		struct Node *client = ArrayList_get(nodes, i);
		if (client->owned_by == NULL) {
			take_node(self, client->ip4addr);
		} else if (!nodes_is_me(client->owned_by)) {
			uint32_t ip4addr = client->ip4addr;
			DNOTIFYA("Getting node (%s) from ", Ip4ToStr(ip4addr));
			DNOTIFYA("(%s)\n", Ip4ToStr(client->owned_by->ip4addr));
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

static void check_sensors_info(Balancer self) {
	size_t sessions_count = HashMap_size(self->sensorSessions);
	struct SensorSession *sessions[sessions_count];
	HashMap_getValues(self->sensorSessions, (void **)sessions);
	time_t now = time(NULL);
	for (size_t i = 0; i < sessions_count; i++) {
		time_t interval = now - sessions[i]->last_info;
		if (interval >= self->session_interval) {
			uint32_t ip4 = sessions[i]->node->ip4addr;
			DNOTIFY("Breaking session with sensor (%s) for timeout (%i)\n", Ip4ToStr(ip4), interval);
			balancing_break_sensor_session(self, ip4);
		} else if (interval >= self->info_interval) {
			InfoRequest inforeq = {INFO_TYPE_POP};
			Services_Request(self->servicesData, InfoService_Get(), 0, &inforeq);
		}
	}

}

static void check_nodes_online(Balancer self) {
	struct Node **nodes = nodes_get();
	size_t nlen = nodes_count();
	time_t now = time(NULL);
	for (size_t i = 0; i < nlen; i++) {
		time_t interval = now - nodes[i]->last_check;
		if (interval >= self->survey_interval) {
			uint32_t ip4 = nodes[i]->ip4addr;
			DINFO("Removing node (%s) for survey timeout (%i)\n", Ip4ToStr(ip4), interval);
			balancing_release_node(self, ip4);
			balancing_break_sensor_session(self, ip4);
			nodes_remove(ip4);
		}
	}
	free(nodes);
}

static void to_STATE_ALONE(Balancer self) {
	self->State = STATE_ALONE;
	take_all_nodes(self);
}

static void to_STATE_COUPLE(Balancer self) {
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
			seek_sensors(self);
			seek_sensors(self);
			self->State = STATE_WAIT_SENSORS;
			break;

		case STATE_WAIT_SENSORS:
			if (!HashMap_size(self->sensorSessions)) {
				self->State = STATE_ALONE;
			} else {
				self->State = STATE_COUPLE;
			}
			repeat = true;
			break;

		case STATE_ALONE:
			debug_owned_clients(self);
			check_nodes_online(self);
			if (!HashMap_size(self->sensorSessions)) {
				take_all_nodes(self);
			} else {
				to_STATE_COUPLE(self);
			}
			break;

		case STATE_COUPLE:
			debug_owned_clients(self);
			check_nodes_online(self);
			if (!HashMap_size(self->sensorSessions)) {
				to_STATE_ALONE(self);
			} else {
				check_sensors_info(self);
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




ArrayList balancing_get_owned(Balancer self) {
	return self->Me.owned;
}

uint8_t balancing_get_state(Balancer self) {
	return self->State;
}

time_t balancing_get_created(Balancer self) {
	return self->Me.created;
}
