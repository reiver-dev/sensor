#include "tcp_connector.hpp"
#include <utility>

using namespace net;

void TcpConnector::connect(EventLoop *loop, char* addr, char* port, const ConnectCB& continuation) {
	entry_t entry;
	Socket sock = Socket::createFromText(addr, port, &entry.addr);
	entry.cb = continuation;

	if (sock.set_nonblocking(true)) {
		throw std::runtime_error(strerror(errno));
	}

	auto res = m_pendingConnections.emplace(sock.fd(), entry);
	ev_io &handler = res.first->second.handler;

	ev_io_init(&handler, onEvent, sock.fd(), EV_READ);
	ev_io_start(loop->get_loop(), &handler);

}

void TcpConnector::setConnectCallback(const ConnectCB& cb) {

}

void TcpConnector::onEvent(struct ev_loop *loop, ev_io *handler, int revent) {
	TcpConnector *self = (TcpConnector *) handler->data;
	auto it = self->m_pendingConnections.find(handler->fd);
	if (it != self->m_pendingConnections.end()) {
		entry_t &e = it->second;
		ev_io_stop(loop, handler);
		StreamLink *channel = e.cb(e.addr);
		if (channel != nullptr) {
			channel->initialize(*handler, self->reactor);
		}
		self->m_pendingConnections.erase(it);
	}
}
