#ifndef TRAFFIC_CAPTURE_H_
#define TRAFFIC_CAPTURE_H_

#include <pcap.h>

#include "sensor_private.hpp"
#include "mq/msgqueue.hpp"


#define PACKET_INFO 0x10
struct PacketInfoRequest {
	size_t size;
};

class TrafficCapture {
public:
	TrafficCapture(sensor_t context, pcap_t *handle, mq::MessageQueue *toCore) :
		active(false), info(context->current), handle(handle), queueToCore(toCore) {}
	void stop();
	bool isRunning();

	static void* start(TrafficCapture *self)  {
		self->run();
		return 0;
	}
private:
	void run();

	bool active;
	InterfaceInfo info;
	pcap_t *handle;
	mq::MessageQueue *queueToCore;
};

void *capture_thread(void *arg);

void TrafficCapture_prepare(struct TrafficCapture *self, sensor_t context, pcap_t *handle);
void *TrafficCapture_start(struct TrafficCapture *self);
void TrafficCapture_stop(struct TrafficCapture *self);
bool TrafficCapture_isRunning(struct TrafficCapture *self);

#endif /* TRAFFIC_CAPTURE_H_ */
