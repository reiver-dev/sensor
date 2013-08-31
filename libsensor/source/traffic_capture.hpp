#ifndef TRAFFIC_CAPTURE_H_
#define TRAFFIC_CAPTURE_H_

#include <pcap.h>

#include "sensor_private.hpp"
#include "raw_packet_handler.hpp"
#include "sensor_service.hpp"

#include <msgqueue.hpp>



#define PACKET_INFO 0x10
struct PacketInfoRequest {
	size_t size;
};

class TrafficCapture {
public:
	TrafficCapture(sensor_t context, RawPacketHandler handler, SensorService::MQ *mqueue) :
		active(false), interface(context->captureInterface), rawHandler(handler), queue(mqueue) {}

	void handlePacket(const struct pcap_pkthdr *packet_header, const uint8_t *packet_data);

	bool isRunning();

	void start();
	void stop();

private:

	bool active;
	InterfaceInfo interface;
	RawPacketHandler rawHandler;
	SensorService::MQ *queue;
};

#endif /* TRAFFIC_CAPTURE_H_ */
