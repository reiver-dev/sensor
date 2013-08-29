#include "sensor_service.hpp"
#include "session_context.hpp"
#include "net/netinfo.h"
#include "base/debug.h"

SensorService::SensorService(sensor_opt_balancing *opts) :
	options(opts), mqueue(this) {

	struct in_addr ip4addr;
	struct in6_addr ip6addr;
	char buffer[64];

	if (get_current_address(AF_INET6,  opts->device_name, &ip6addr)) {
		currentAddress.setAddr6(ip6addr);
	} else if (get_current_address(AF_INET, opts->device_name, &ip4addr)) {
		currentAddress.setAddr4(ip4addr);
	} else {
		DERROR("Error getting interface (%s) address", opts->device_name);
	}

	acceptor.initialize(currentAddress.toString(buffer), "33178");
	acceptor.start(&eventLoop);

}

void SensorService::start() {
	eventLoop.start();
}

void SensorService::stop() {
	eventLoop.stop();
}

/*void SensorService::accepted(ev::io &watcher, int events) {
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

}*/

