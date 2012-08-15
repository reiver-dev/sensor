#ifndef TRAFFIC_CAPTURE_H_
#define TRAFFIC_CAPTURE_H_

#include "sensor_private.h"
#include "message_queue.h"

struct TrafficCapture {
	sensor_t context;
	MessageQueue *queueToCore;
	MessageQueue *queueToPersist;
};

void *capture_thread(void *arg);



#endif /* TRAFFIC_CAPTURE_H_ */
