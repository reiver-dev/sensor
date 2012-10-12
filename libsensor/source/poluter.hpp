#ifndef POLUTER_H_
#define POLUTER_H_

#include <pcap.h>

#include "sensor_private.hpp"
#include "message_queue.hpp"
#include "netinfo.hpp"



class Poluter {
public:

	struct MsgSpoof {
		size_t target_count;
		struct NetAddress *targets;
	};

	void perform_survey();
	void spoof_nodes(MsgSpoof *msg);

	Poluter(sensor_t context, pcap_t *handle) :
		active(false), handle(handle), info(context->current) {}

private:
	void run();

	bool active;
	pcap_t *handle;
	struct InterfaceInfo info;
};

#endif /* POLUTER_H_ */
