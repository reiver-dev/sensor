#ifndef RAW_PACKET_HANDLER_HPP_
#define RAW_PACKET_HANDLER_HPP_

#include <pcap/pcap.h>
#include <cstdint>
#include <utility>

#include "base/debug.h"

class RawPacketHandler {
public:

	typedef void (*handler_f)(void *data, const pcap_pkthdr *packet_header, const uint8_t *packet_data);

	RawPacketHandler() : m_handler(nullptr) {

	}

	RawPacketHandler(const RawPacketHandler& o) : m_handler(o.m_handler) {

	}

	~RawPacketHandler() {

	}

	bool initialize(const char *deviceName, size_t bufferSize, bool promiscuous);
	void close() {
		if (isInitialized()) {
			pcap_close(m_handler);
		}
	}

	bool isInitialized() {
		return m_handler != nullptr;
	}

	ssize_t capture(pcap_pkthdr **packet_header, const uint8_t **packet_data) {
		int res = pcap_next_ex(m_handler, packet_header, packet_data);
		return res;
	}

	ssize_t inject(const uint8_t *buffer, size_t length) {
		int res = pcap_inject(m_handler, buffer, length);
		return res;
	}

	char *error() {
		return pcap_geterr(m_handler);
	}

	void start(void* data, handler_f callback);

	void stop();

private:

	RawPacketHandler(RawPacketHandler&& o) = delete;

	pcap_t *m_handler;

};
#endif /* RAW_PACKET_HANDLER_HPP_ */
