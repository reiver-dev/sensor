#ifndef NEGOTIATOR_HPP_
#define NEGOTIATOR_HPP_

#include <unordered_map>

#include <ev++.h>

#include "sensor_private.hpp"
#include "socket_utils.hpp"
#include "session_context.hpp"

class SensorService {
public:


	SensorService(sensor_opt_balancing *opts);

	virtual ~SensorService();

	void run();

private:

	void connect_accepted(ev::io &watcher, int events);
	void lookup_received(ev::io &watcher, int events);
	void polute_timeout(ev::timer &watcher);

	sensor_opt_balancing *options;
	int server_socket;

	ev::default_loop loop;

	ev::io connect_watcher;
	ev::io lookup_watcher;

	ev::timer balance_timer;

	std::unordered_map<InternetAddress, SessionContext> sessions;

};

#endif /* NEGOTIATOR_HPP_ */
