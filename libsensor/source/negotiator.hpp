#ifndef NEGOTIATOR_HPP_
#define NEGOTIATOR_HPP_

#include <ev++.h>

#include "sensor_private.hpp"
#include "socket_utils.hpp"

class Negotiator {
public:


	Negotiator(sensor_opt_balancing *opts);


	virtual ~Negotiator();



private:
	sensor_opt_balancing *options;
	ev::dynamic_loop loop;
	int server_socket;

};

#endif /* NEGOTIATOR_HPP_ */
