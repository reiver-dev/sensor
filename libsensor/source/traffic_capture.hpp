#ifndef TRAFFIC_CAPTURE_H_
#define TRAFFIC_CAPTURE_H_

#include <pcap.h>

#include "sensor_private.hpp"
#include "raw_packet_handler.hpp"

#include <msgqueue.hpp>



#define PACKET_INFO 0x10
struct PacketInfoRequest {
	size_t size;
};

class TrafficCapture {
public:
	TrafficCapture(sensor_t context, RawPacketHandler handler, mq::MessageQueue *toCore) :
		active(false), interface(context->captureInterface), rawHandler(handler), queueToCore(toCore) {}

	void handlePacket(const struct pcap_pkthdr *packet_header, const uint8_t *packet_data);

	bool isRunning();

	static void *start(TrafficCapture *self);
	void stop();

private:

	bool active;
	InterfaceInfo interface;
	RawPacketHandler rawHandler;
	mq::MessageQueue *queueToCore;
};

void *capture_thread(void *arg);

void TrafficCapture_prepare(struct TrafficCapture *self, sensor_t context, pcap_t *handle);
void *TrafficCapture_start(struct TrafficCapture *self);
void TrafficCapture_stop(struct TrafficCapture *self);
bool TrafficCapture_isRunning(struct TrafficCapture *self);

#endif /* TRAFFIC_CAPTURE_H_ */
