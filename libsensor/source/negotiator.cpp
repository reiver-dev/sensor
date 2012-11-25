#include "negotiator.hpp"


Negotiator::Negotiator(sensor_opt_balancing *opts)
		: options(opts)
		, loop(ev::NOENV)
	{
		int s = socket(AF_INET, SOCK_DGRAM, 0);
		get_current_address(s, opts->device_name);
		server_socket = create_tcp_server("0.0.0.0", "31337");
		listen(server_socket, SOMAXCONN);

	}

Negotiator::~Negotiator() {
	// TODO Auto-generated destructor stub
}

