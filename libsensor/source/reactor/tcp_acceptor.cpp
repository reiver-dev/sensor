#include "tcp_acceptor.hpp"

#include <stdexcept>

using namespace net;

TcpAcceptor::TcpAcceptor(const char *addr, const char *port) {
	sock = Socket::createFromText(addr, port, &m_address);

	if (sock.set_nonblocking(true)) {
		throw std::runtime_error(strerror(errno));
	}

	if (sock.setReuseAddr(true)) {
		throw std::runtime_error(strerror(errno));
	}

	if (sock.bind(m_address)) {
		throw std::runtime_error(strerror(errno));
	}

	ev_io_init(&handler, onEvent, sock.fd(), EV_READ);
	handler.data = this;
	reactor = 0;
}

void TcpAcceptor::setAcceptCallback(const AcceptCB& cb) {
	onAccept = cb;
}

void TcpAcceptor::start(EventLoop *r) {
	reactor = r;
	if (sock.listen()) {
		throw std::runtime_error(strerror(errno));
	}
	std::cout << "listening on " << m_address << std::endl;
	ev_io_start(reactor->get_loop(), &handler);
}

void TcpAcceptor::stop() {
	ev_io_stop(reactor->get_loop(), &handler);
}

void TcpAcceptor::onEvent(EV_P_ ev_io *handler, int revent) {
	TcpAcceptor *self = (TcpAcceptor *)handler->data;
	EndpointAddress address;
	Socket sock = self->sock.accept(&address);
	StreamLink *channel = self->onAccept(address);
	if (channel != nullptr)
		channel->initialize(sock, self->reactor);
}
