#ifndef NEGOTIATOR_HPP_
#define NEGOTIATOR_HPP_

#include "sensor_private.hpp"
#include "node.hpp"
#include "poluter.hpp"

#include "reactor/reactor.hpp"
#include "service/async_msgqueue.hpp"



class SensorService {
public:

	typedef MemberAsyncQueue<SensorService> MQ;

	SensorService(sensor_t config);

	void setPolluterMq(Poluter::MQ *mq) {
		polluterMqueue = mq;
	}

	MQ *messageQueue() {
		return &mqueue;
	}

	void start();
	void stop();

	void onNodeFound(const Node &node);
	net::StreamLink* onConnectionAccepted(const net::EndpointAddress &addr);
	void onDataRead(net::StreamLink &link);

	net::StreamLink* onConnectionEstablished(const net::EndpointAddress &addr);

private:

	sensor_opt_balancing *options;
	InternetAddress currentAddress;

	MQ mqueue;
	Poluter::MQ *polluterMqueue;

	net::EventLoop eventLoop;
	net::TcpAcceptor acceptor;
	net::TcpConnector connector;

	struct get_hash {
		size_t operator()(const net::EndpointAddress& o) const {
			size_t h = 0;
			size_t s = sizeof(net::EndpointAddress);
			char *data = (char *)&o;
			while(s) {
				h += 33 * h + *data++;
				s--;
			}
			return h;
		}
	};

	std::unordered_map<net::EndpointAddress, net::StreamLink, get_hash> activeChannels;

};

#endif /* NEGOTIATOR_HPP_ */
