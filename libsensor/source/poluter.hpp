#ifndef POLUTER_H_
#define POLUTER_H_

#include <pcap.h>
#include <iostream>

#include "sensor_private.hpp"
#include <msgqueue.hpp>
#include "netinfo.hpp"



class Poluter {
public:

	struct MsgSpoof {
		size_t target_count;
		struct NetAddress *targets;

		MsgSpoof() : target_count(0), targets(nullptr) {
			std::cout << "CONSTR" << std::endl;
		}

		MsgSpoof(const MsgSpoof &msg) : target_count(msg.target_count), targets(msg.targets) {
			std::cout << "COPY" << std::endl;
		}

		MsgSpoof(MsgSpoof &&msg) : target_count(msg.target_count), targets(msg.targets) {
			std::cout << "MOVE" << std::endl;
		}

		MsgSpoof& operator=(const MsgSpoof &msg) {
			target_count = msg.target_count;
			targets = msg.targets;
			std::cout << "COPY" << std::endl;
			return *this;
		}

		MsgSpoof& operator=(MsgSpoof &&msg) {
			target_count = msg.target_count;
			targets = msg.targets;
			std::cout << "MOVE" << std::endl;
			return *this;
		}

	};

	void perform_survey();
	int spoof_nodes(MsgSpoof msg);

	Poluter(sensor_t context, pcap_t *handle) :
		active(false), handle(handle), info(context->current) {}

private:
	void run();

	bool active;
	pcap_t *handle;
	struct InterfaceInfo info;
};

#endif /* POLUTER_H_ */
