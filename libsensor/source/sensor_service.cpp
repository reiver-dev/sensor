#include "sensor_service.hpp"
#include "net/netinfo.h"
#include "base/debug.h"

SensorService::SensorService(sensor_t config) :
	options(&config->opt.balancing), mqueue(this) {

	struct in_addr ip4addr;
	struct in6_addr ip6addr;
	char buffer[64];

	if (get_current_address(AF_INET6, options->device_name, &ip6addr)) {
		currentAddress.setAddr6(ip6addr);
	} else if (get_current_address(AF_INET, options->device_name, &ip4addr)) {
		currentAddress.setAddr4(ip4addr);
	} else {
		DERROR("Error getting interface (%s) address", options->device_name);
	}

	acceptor.init(currentAddress.toString(buffer), "33178",
		CB_BIND_MEM(this, &SensorService::onConnectionAccepted));
	acceptor.start(&eventLoop);

}

void SensorService::start() {
	eventLoop.start();
}

void SensorService::stop() {
	eventLoop.stop();
}

void SensorService::onNodeFound(const Node &node) {

}

net::StreamLink* SensorService::onConnectionAccepted(const net::EndpointAddress &addr) {
	auto it = activeChannels.emplace(addr, addr);
	if (it.second) {
		net::StreamLink *link = &it.first->second;
		link->setReadCallback(CB_BIND_MEM(this, &SensorService::onDataRead));
		link->setReceiving(true);
		return link;
	}
	return nullptr;
}

void SensorService::onDataRead(net::StreamLink &link) {

}

net::StreamLink* SensorService::onConnectionEstablished(const net::EndpointAddress &addr) {
	auto it = activeChannels.emplace(addr, addr);
	if (it.second) {
		net::StreamLink *link = &it.first->second;
		link->setReadCallback(CB_BIND_MEM(this, &SensorService::onDataRead));
		link->setReceiving(true);
		return link;
	}
	return nullptr;
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

