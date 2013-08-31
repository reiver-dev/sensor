#include "raw_packet_handler.hpp"
#include "base/debug.h"

bool RawPacketHandler::init(const char *deviceName, size_t bufferSize, bool promiscuous) {
	char errbuf[PCAP_ERRBUF_SIZE] = {0};
	pcap_t *pcapHandle = NULL;

	bool success = true;
	pcapHandle = pcap_create(deviceName, errbuf);
	DINFO("Pcap created %s\n", errbuf);

	success = !pcap_set_promisc(pcapHandle, promiscuous);
	success = success && !pcap_set_buffer_size(pcapHandle, bufferSize);

	if (!pcap_setdirection(pcapHandle, PCAP_D_OUT)) {
		DWARNING("%s\n", "can't set PCAP_D_OUT");
	}

	success = success && !pcap_activate(pcapHandle);

	if (success) {
		m_handler = pcapHandle;
		DINFO("Pcap initialized successfully\n", errbuf);
	} else {
		pcap_close(pcapHandle);
		DERROR("Pcap crash: %s\n", pcap_geterr(pcapHandle));
	}

	return success;
}

void RawPacketHandler::start(void* data, handler_f callback) {
	int res = pcap_dispatch(m_handler, -1, (pcap_handler) (callback), (uint8_t*)data);
	if (res == -1) {
		char *err = pcap_geterr(m_handler);
		DERROR("Pcap loop error: %s", err);
	} else {
		DINFO("%s", "Pcap exited successfully");
	}
}

void RawPacketHandler::stop() {
	pcap_breakloop(m_handler);
}
