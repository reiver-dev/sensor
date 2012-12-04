#include <ev++.h>

#include "negotiator.hpp"
#include "session_context.hpp"

class SensorWatcher {
public:

	SensorWatcher(int s) : fd(s) {
		set_nonblocking(fd);
		session_watcher.set<SensorWatcher, &SensorWatcher::callback>(this);
		session_watcher.start(fd, ev::READ | ev::WRITE);
	}

private:

	void read_callback(ev::io &watcher, int events) {

	}

	void write_callback(ev::io &watcher, int events) {

	}

	void callback(ev::io &watcher, int events) {
		if (EV_ERROR & events) {
			return;
		}

		if (events & EV_READ)
			read_callback(watcher, events);

		if (events & EV_WRITE)
			write_callback(watcher, events);

	}


	ev::io session_watcher;
	int fd;
	SessionContext context;
};



Negotiator::Negotiator(sensor_opt_balancing *opts) :
	options(opts), loop(ev::NOENV)
{
		int s = socket(AF_INET, SOCK_DGRAM, 0);
		get_current_address(s, opts->device_name);
		server_socket = create_tcp_server("0.0.0.0", "31337");
		listen(server_socket, SOMAXCONN);
}

void Negotiator::run() {
	connect_watcher.set<Negotiator, &Negotiator::accept_callback>(this);
	connect_watcher.start(server_socket, ev::READ);


}

void Negotiator::accept_callback(ev::io &watcher, int events) {
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	accept(watcher.fd, (struct sockaddr *) &addr, &addr_len);
}

Negotiator::~Negotiator() {
	// TODO Auto-generated destructor stub
}

