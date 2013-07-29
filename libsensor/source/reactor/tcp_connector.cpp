#include "tcp_connector.hpp"
#include <utility>

using namespace net;

void TcpConnector::connect(char* addr, char* port, const ConnectCB& continuation) {
	EndpointAddress resolved = EndpointAddress();
	Socket sock = Socket::createFromText(addr, port, &resolved);

	if (sock.set_nonblocking(true)) {
		throw std::runtime_error(strerror(errno));
	}

	ev_io handler;
	ev_io_init(&handler, onEvent, sock.fd(), EV_READ);

	m_pendingConnections.emplace(handler, std::make_pair(resolved, continuation));
}

void TcpConnector::setConnectCallback(const ConnectCB& cb) {
}

void TcpConnector::onEvent(struct ev_loop *loop, ev_io *handler, int revent) {
	TcpConnector *self = (TcpConnector *) handler->data;
	auto it = self->m_pendingConnections.find(*handler);
	if (it != self->m_pendingConnections.end()) {
		entry_t &e = it->second;
		ev_io_stop(loop, handler);
		StreamLink *channel = e.second(e.first);
		if (channel != nullptr) {
			channel->initialize(*handler, self->reactor);
		}
		self->m_pendingConnections.erase(it);
	}
}
