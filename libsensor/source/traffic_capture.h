#ifndef TRAFFIC_CAPTURE_H_
#define TRAFFIC_CAPTURE_H_

#include <pcap.h>

#include "sensor_private.h"
#include "message_queue.h"

struct TrafficCapture {
	bool active;
	pcap_t *handle;
	MessageQueue *queueToCore;
	MessageQueue *queueToPersist;
};

void *capture_thread(void *arg);

bool TrafficCapture_prepare(struct TrafficCapture *self, sensor_t context);
void TrafficCapture_close(struct TrafficCapture *self);
void *TrafficCapture_start(struct TrafficCapture *self);
void TrafficCapture_stop(struct TrafficCapture *self);
bool TrafficCapture_isRunning(struct TrafficCapture *self);

#endif /* TRAFFIC_CAPTURE_H_ */
