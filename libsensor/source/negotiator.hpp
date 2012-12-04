#ifndef NEGOTIATOR_HPP_
#define NEGOTIATOR_HPP_

#include <unordered_map>

#include "sensor_private.hpp"
#include "socket_utils.hpp"
#include "session_context.hpp"

class Negotiator {
public:


	Negotiator(sensor_opt_balancing *opts);

	virtual ~Negotiator();

	void run();

private:

	void accept_callback(ev::io &watcher, int events);


	sensor_opt_balancing *options;
	int server_socket;

	ev::default_loop loop;
	ev::io connect_watcher;
	ev::io read_watcher;

	std::unordered_map<Address, SessionContext> sessions;

};

#endif /* NEGOTIATOR_HPP_ */
