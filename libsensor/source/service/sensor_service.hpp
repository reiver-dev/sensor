#ifndef NEGOTIATOR_HPP_
#define NEGOTIATOR_HPP_


#include "reactor/reactor.hpp"

#include "sensor_private.hpp"
#include "negotiation_model.hpp"
#include "async_msgqueue.hpp"



class SensorService {
public:

	SensorService(sensor_opt_balancing *opts);

	void start();
	void stop();

private:

	sensor_opt_balancing *options;
	InternetAddress currentAddress;

	MemberAsyncQueue<SensorService> mqueue;

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
	NegotiationModel model;

};

#endif /* NEGOTIATOR_HPP_ */
