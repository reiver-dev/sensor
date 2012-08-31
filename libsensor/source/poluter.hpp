#ifndef POLUTER_H_
#define POLUTER_H_

#include <pcap.h>

#include "sensor_private.hpp"
#include "message_queue.hpp"
#include "netinfo.hpp"

enum {
	POLUTER_MSG_SPOOF,
	POLUTER_MSG_SURVEY
};

struct PoluterMsgSpoof {
	size_t target_count;
	struct NetAddress *targets;
};

class Poluter {
public:
	Poluter(sensor_t context, pcap_t *handle, MessageQueue toCore) :
		active(false), handle(handle), info(context->current), queueToCore(toCore) {}
	void stop();
	bool isRunning();

	static void* start(Poluter *self)  {
		self->run();
		return 0;
	}
private:
	void run();
	void perform_survey();
	void spoof_nodes(struct NetAddress *targets, size_t targets_count);

	bool active;
	pcap_t *handle;
	struct InterfaceInfo info;
	MessageQueue queueToCore;
};


void Poluter_prepare(struct Poluter *self, sensor_t context, pcap_t *handle);
void *Poluter_start(struct Poluter *self);
void Poluter_stop(struct Poluter *self);
bool Poluter_isRunning(struct Poluter *self);

#endif /* POLUTER_H_ */
