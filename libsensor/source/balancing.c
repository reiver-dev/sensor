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

#include "sensor.h"
#include "survey.h"
#include "debug.h"
#include "util.h"
#include "nodes.h"

#include "services/info.h"


#define ME_JUST_ARRIVED 0
#define ME_ALONE 1

static int udp_sock;

static uint8_t State;

static Service services[12];

static struct {
	uint32_t ip4;
	uint32_t network;
	uint8_t hw[ETH_ALEN];
} current;

bool is_same_network_ip4(uint32_t ip) {
	return (current.network & ip) == current.network;
}

/* ---------------------------------------------- */
void balancing_init(sensor_t *config) {
	/* memorize current addreses */
	current.ip4 = config->ip4addr;
	memcpy(current.hw, config->hwaddr, ETH_ALEN);
	current.network = config->ip4addr & config->netmask;

	nodes_init(config->ip4addr, config->netmask);

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(31337);
	bind(udp_sock, &sockaddr, sizeof(sockaddr));

	services[0] = get_info_service();

}

void balancing_survey(int packet_sock) {
	int length;
	uint8_t *survey_buf = survey_packet(&length, 0, current.ip4, current.hw);

	assert(survey_buf != 0);
	assert(length > 0);

	int result;
	int nodeCount = nodes_count();
	struct Node *nodes = nodes_get();
	for (int i = 0; i < nodeCount;  i++) {
		survey_set_target_ip(survey_buf, nodes[i].ip4addr);
		DINFO("Send survey to: %s\n", Ip4ToStr(nodes[i].ip4addr));
		result = send(packet_sock, survey_buf, length, 0);
		if (result == -1) {
			DERROR("%s\n", "Failed to send survey");
		}
	}
}

void balancing_check_response(uint8_t *buffer, int length) {
	survey_process_response(buffer, length);
}


/*-------------------------------------*/

void service_invoke(int sock, uint32_t serviceID, void *request) {
	int serv_count = sizeof(services) / sizeof(Service);
	for(int i = 0; i < serv_count; i++) {
		if (services[i].Name == serviceID) {
			services[i].Request(sock, request);
		}
	}
}


void seek_sensors() {
	InfoRequest request;
	request.node = 0;
	service_invoke(udp_sock, SERVICE_INFO, &request);
}


void balancing_process(int sock) {

	switch(State) {
	case ME_JUST_ARRIVED:
		seek_sensors(sock);
		break;


	}


}


void balancing_destroy() {
	shutdown(udp_sock, 2);
	nodes_destroy();
}

