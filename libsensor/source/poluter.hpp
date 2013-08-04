#ifndef POLUTER_H_
#define POLUTER_H_

#include <pcap.h>
#include <iostream>

#include "sensor_private.hpp"
#include <member_msgqueue.hpp>
#include "net/netinfo.h"
#include "raw_packet_handler.hpp"



class Poluter {
public:

	struct MsgSpoof {
		size_t target_count;
		struct NodeAddress *targets;

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

	static void *start(Poluter *self);

	void perform_survey();
	int spoof_nodes(MsgSpoof msg);

	mq::MemberMessageQueue<Poluter>& messageQueue() {
		return message_queue;
	}

	Poluter(sensor_t context, RawPacketHandler handler) :
		active(false), rawHandler(handler), info(context->captureInterface), message_queue(this) {}

private:
	void run();

	bool active;
	RawPacketHandler rawHandler;
	struct InterfaceInfo info;
	mq::MemberMessageQueue<Poluter> message_queue;
};

#endif /* POLUTER_H_ */
