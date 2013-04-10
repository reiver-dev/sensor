#ifndef NEGOTIATOR_HPP_
#define NEGOTIATOR_HPP_

#include <ev++.h>

#include "sensor_private.hpp"
#include "socket_utils.h"
#include "sensor_watcher.hpp"
#include "negotiation_model.hpp"
#include "event_system.hpp"

#include <signal/signaled_member_queue.hpp>


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

	IoEvent connect_watcher;
	IoEvent accept_watcher;
	IoEvent lookup_watcher;
	IoEvent capture_watcher;

	TimerEvent balance_timer;
	TimerEvent lookup_timer;
	TimerEvent survey_timer;

	NegotiationModel model;

};

#endif /* NEGOTIATOR_HPP_ */
