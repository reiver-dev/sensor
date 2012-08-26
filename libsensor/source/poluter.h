#ifndef POLUTER_H_
#define POLUTER_H_

#include <pcap.h>

#include "sensor_private.h"
#include "message_queue.h"
#include "tshashmap.h"
#include "netinfo.h"

enum {
	POLUTER_MSG_SPOOF,
	POLUTER_MSG_SURVEY
};

struct PoluterMsgSpoof {
	size_t target_count;
	struct NetAddress *targets;
};


struct Poluter {
	bool active;
	pcap_t *handle;
	struct InterfaceInfo info;
	MessageQueue queueToCore;
	TsHashMap nodes;
	TsHashMap sessions;
};

void Poluter_prepare(struct Poluter *self, sensor_t context, pcap_t *handle);
void *Poluter_start(struct Poluter *self);
void Poluter_stop(struct Poluter *self);
bool Poluter_isRunning(struct Poluter *self);

#endif /* POLUTER_H_ */
