#include <stdlib.h>
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

#include <arpa/inet.h>

#include "balancing.h"
#include "survey.h"
#include "debug.h"
#include "util.h"
#include "nodes.h"
#include "spoof.h"

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
	uint8_t State;
	struct CurrentAddress *current;
};


bool is_same_network_ip4(Balancer self, uint32_t ip) {
	uint32_t network = self->current->ip4addr & self->current->netmask;
	return (network & ip) == network;
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

void balancing_survey(Balancer self, int packet_sock) {
	DINFO("%s\n", "Starting survey");

	int length;
	uint8_t *survey_buf = survey_packet(&length, 0, self->current->ip4addr, self->current->hwaddr);

	assert(survey_buf != 0);
	assert(length > 0);

	int result;
	int nodeCount = nodes_count();
	struct Node *nodes = nodes_get();
	for (int i = 0; i < nodeCount;  i++) {
		survey_set_target_ip(survey_buf, nodes[i].ip4addr);
		result = send(packet_sock, survey_buf, length, 0);
		if (result == -1) {
			DERROR("Failed to send survey to %s\n", Ip4ToStr(nodes[i].ip4addr));
		}
	}


	DINFO("%s\n", "Survey finished");
}


void balancing_modify(Balancer self, int packet_sock) {
	DINFO("%s\n", "Starting spoofing");
	Spoof_nodes(packet_sock, self->current);
	DINFO("%s\n", "Spoofing finished");
}

void balancing_check_response(Balancer self, uint8_t *buffer, int length) {
	survey_process_response(buffer, length, self->current);
}


/*-------------------------------------*/

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


void balancing_destroy(Balancer self) {
	shutdown(self->udp_sock, 2);
	free(self);
}

