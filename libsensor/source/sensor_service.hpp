#ifndef NEGOTIATOR_HPP_
#define NEGOTIATOR_HPP_

#include <ev++.h>

#include "sensor_private.hpp"
#include "socket_utils.hpp"
#include "session_context.hpp"
#include "signaled_member_queue.hpp"
#include "negotiation_model.hpp"

class SensorService {
public:


	SensorService(sensor_opt_balancing *opts);

	virtual ~SensorService();

	void initialize();
	void run();


private:

	void accepted(ev::io &watcher, int events);
	void connected(ev::io &watcher, int events);
	void lookup_received(ev::io &watcher, int events);
	void polute_timeout(ev::timer &watcher);

	sensor_opt_balancing *options;
	SignaledMemberQueue<SensorService> mqueue;

	int server_socket;
	int lookup_socket;

	ev::default_loop loop;

	ev::io connect_watcher;
	ev::io accept_watcher;
	ev::io lookup_watcher;
	ev::io capture_watcher;

	ev::timer balance_timer;
	ev::timer lookup_timer;
	ev::timer survey_timer;

	NegotiationModel model;

};

#endif /* NEGOTIATOR_HPP_ */
