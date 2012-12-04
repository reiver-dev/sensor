#include <stdbool.h>
#include <pcap.h>

#include "networking.h"

#include "traffic_capture.hpp"
#include "debug.hpp"
#include "arputil.hpp"
#include "node.hpp"



void TrafficCapture::run() {
	int res;
	const uint8_t *packet_data;
	struct pcap_pkthdr *packet_header;
	struct PacketInfoRequest pInfoRequest = {0};

	active = true;

	while (active) {
		res = pcap_next_ex(handle, &packet_header, &packet_data);
		if (res < 0) {
			pcap_perror(handle, (char *)"\n");
			active = false;
		} else if (res > 0) {
			pInfoRequest.size = packet_header->len;
			DINFO("Captured inbound %i\n", pInfoRequest.size);
			queueToCore->send(sensor_log_packet, (int)packet_header->len);

			struct NodeAddress node;
			if (arp_is_reply(packet_data, packet_header->len, &info, &node.in.s_addr, node.hw.ether_addr_octet)) {
				DINFO("Got reply from %s\n", inet_ntoa(node.in));
			}

		}
	}
	queueToCore->nullmsg();
	return;
}

void TrafficCapture::stop() {
	active = false;
}

bool TrafficCapture::isRunning() {
	return active;
}
