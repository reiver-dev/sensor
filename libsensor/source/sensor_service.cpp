#include <ev++.h>

#include "sensor_service.hpp"
#include "session_context.hpp"


SensorService::SensorService(sensor_opt_balancing *opts) :
	options(opts), loop(ev::NOENV)
{
		int s = socket(AF_INET, SOCK_DGRAM, 0);
		get_current_address(s, opts->device_name);
		server_socket = create_tcp_server("0.0.0.0", "31337");
		listen(server_socket, SOMAXCONN);
}

void SensorService::run() {
	connect_watcher.set<SensorService, &SensorService::connect_accepted>(this);
	connect_watcher.start(server_socket, ev::READ);

}

void SensorService::connect_accepted(ev::io &watcher, int events) {
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	accept(watcher.fd, (struct sockaddr *) &addr, &addr_len);
}

SensorService::~SensorService() {
	// TODO Auto-generated destructor stub
}

