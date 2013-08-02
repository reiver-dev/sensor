#include <ev++.h>
#include <unistd.h>
#include <sys/socket.h>

#include "sensor_service.hpp"
#include "session_context.hpp"
#include "net/netinfo.h"


SensorService::SensorService(sensor_opt_balancing *opts) :
	options(opts), mqueue(nullptr), loop(ev::NOENV)
{

	int s = socket(AF_INET, SOCK_DGRAM, 0);
	get_current_address(s, opts->device_name);
	close(s);

	server_socket = create_socket_server(SOCK_STREAM, "0.0.0.0", "31337");
	set_nonblocking(server_socket);
	listen(server_socket, SOMAXCONN);

	lookup_socket = create_socket_server(SOCK_DGRAM, "0.0.0.0", "31338");
	set_nonblocking(lookup_socket);

	accept_watcher.set<SensorService, &SensorService::accepted>(this);
	accept_watcher.start(server_socket, ev::READ);

	lookup_watcher.set<SensorService, &SensorService::lookup_received>(this);
	lookup_watcher.start(lookup_socket, ev::READ);

}

SensorService::~SensorService() {

}

void SensorService::run() {
	loop.run(0);



}

void SensorService::accepted(ev::io &watcher, int events) {
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	int fd = accept(watcher.fd, (struct sockaddr *) &addr, &addr_len);
	InternetAddress from((struct sockaddr *)&addr, addr_len);
	SessionContext *session;
	if (!model.check_session(from)) {
		session = model.create_session(from);
	} else {
		session = model.get_session(from);
	}
	SensorWatcher *sensor_watcher = SensorWatcher::createInstance(session, fd);
	sensor_watcher->start();

}

void SensorService::connected(ev::io &watcher, int events) {

}

void SensorService::lookup_received(ev::io &watcher, int events) {

}

void SensorService::polute_timeout(ev::timer &watcher) {

}

