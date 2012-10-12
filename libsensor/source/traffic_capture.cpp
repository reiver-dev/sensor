#include <stdbool.h>
#include <pcap.h>

#include "traffic_capture.hpp"
#include "debug.hpp"
#include "arputil.hpp"
#include "util.hpp"




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
			queueToCore->request((MessageQueue::callback) sensor_set_persist_callback, &packet_header->len);

			struct NetAddress node;
			if (arp_is_reply(packet_data, packet_header->len, &info, &node)) {
				DINFO("Got reply from %s\n", Ip4ToStr(node.in));
			}

		}
	}
	return;
}

void TrafficCapture::stop() {
	active = false;
}

bool TrafficCapture::isRunning() {
	return active;
}
