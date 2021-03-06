#include <stdbool.h>
#include <pcap.h>

#include <net/networking.h>
#include <base/debug.h>

#include "traffic_capture.hpp"

#include "net/arputil.h"
#include "node.hpp"

static void handle_packet_callback(void *data, const struct pcap_pkthdr *header, const uint8_t *buffer) {
	static_cast<TrafficCapture *>(data)->handlePacket(header, buffer);
}

void TrafficCapture::start() {
	active = true;
	rawHandler.start(this, handle_packet_callback);
	active = false;
	queue->nullmsg();
}


void TrafficCapture::handlePacket(const struct pcap_pkthdr *packet_header, const uint8_t *packet_data) {
	struct PacketInfoRequest pInfoRequest = { 0 };
	pInfoRequest.size = packet_header->len;
	DINFO("Captured inbound %i\n", pInfoRequest.size);

	struct in_addr ip4;
	uint8_t hw[ETH_ALEN];
	if (arp_is_reply(packet_data, packet_header->len, interface.hw.ether_addr_octet, &ip4.s_addr, hw)) {
		DINFO("Got reply from %s\n", inet_ntoa(ip4));
	}

}

void TrafficCapture::stop() {
	rawHandler.stop();
}

bool TrafficCapture::isRunning() {
	return active;
}
