#include <stdbool.h>
#include <pcap.h>

#include "traffic_capture.hpp"
#include "debug.hpp"
#include "arputil.hpp"
#include "util.hpp"

void TrafficCapture_prepare(struct TrafficCapture *self, sensor_t context, pcap_t *handle) {
	self->active = false;
	self->info = context->current;
	self->handle = handle;
}

void *TrafficCapture_start(struct TrafficCapture *self) {
	int res;
	const uint8_t *packet_data;
	struct pcap_pkthdr *packet_header;
	struct PacketInfoRequest pInfoRequest = {0};

	self->active = true;

	while (self->active) {
		res = pcap_next_ex(self->handle, &packet_header, &packet_data);
		if (res < 0) {
			pcap_perror(self->handle, (char *)"\n");
			self->active = false;
			return 0;
		} else if (res > 0) {
			pInfoRequest.size = packet_header->len;
			DINFO("Captured inbound %i\n", pInfoRequest.size);
			MessageQueue_send(self->queueToCore, packet_header->len, NULL, 0);

			struct NetAddress node;
			if (arp_is_reply(packet_data, packet_header->len, &self->info, &node)) {
				DINFO("Got reply from %s\n", Ip4ToStr(node.in));
			}

		}
	}
	return 0;
}

void TrafficCapture_stop(struct TrafficCapture *self) {
	self->active = false;
}

bool TrafficCapture_isRunning(struct TrafficCapture *self) {
	return self->active;
}
