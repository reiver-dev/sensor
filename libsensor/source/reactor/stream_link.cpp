#include "stream_link.hpp"
#include "socket.hpp"
#include "utils.hpp"

using namespace net;

StreamLink::~StreamLink() {

}


void StreamLink::onEvent(EV_P_ ev_io *handler, int revents) {
	StreamLink *link = static_cast<StreamLink*>(handler->data);
	if (revents || EV_READ) {
		link->processRead();
	}
	if ((revents || EV_WRITE) && (link->m_state == State::OK)) {
		link->processWrite();
	}

	if (!link->prepareEvents()) {
		link->stop();
		link->destroy();
		link->onDispose(*link);
	}
}

void StreamLink::setReadCallback(const Callback& cb) {
	onRead = cb;
}

void StreamLink::setWriteCallback(const Callback& cb) {
	onWriteCompleted = cb;
}

void StreamLink::setDisposeCallback(const Callback& cb) {
	onDispose = cb;
}

void StreamLink::peek(char **d, size_t *l) {
	*d = m_in.readPtr();
	*l = m_in.pending();
}

void StreamLink::advance(size_t l) {
	m_in.processed(l);
}

void StreamLink::write(char *data, size_t len) {
	m_out.append(data, len);
}

void StreamLink::init(Socket sock) {
	m_sock = sock;
	m_handler.data = this;
	ev_init(&m_handler, onEvent);
}

void StreamLink::init(const ev_io &handler) {
	m_sock = handler.fd;
	m_handler.data = this;
	ev_set_cb(&m_handler, onEvent);
}

void StreamLink::destroy() {
	if (m_sock.fd()) {
		m_sock.close();
	}
}

void StreamLink::start(EventLoop *reactor) {
	m_reactor = reactor;
	if (prepareEvents())
		ev_io_start(m_reactor->get_loop(), &m_handler);
}

void StreamLink::stop() {
	ev_io_stop(m_reactor->get_loop(), &m_handler);
}

bool StreamLink::prepareEvents() {
	int events = 0;

	if (m_state == State::DISCONNECT || m_state == State::READ_ERROR) {
		return false;
	}

	if (isReceiving() && (m_state == State::OK && m_state == State::WRITE_ERROR)) {
		events |= EV_READ;
	}

	if (isSending() && m_state == State::OK) {
		events |= EV_WRITE;
	}

	m_reactor->set_io_events(&m_handler, events);
	return true;

}

void StreamLink::processRead() {
	size_t got = Sock::bufferedRecv(m_sock, m_in);

	if (got == (size_t) -1 && !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
		m_state = State::READ_ERROR;
		saved_errno = errno;
	}

	if (got == 0) {
		m_state = State::DISCONNECT;
	}

	if (got > 0 && m_in.pending()) {
		onRead(*this);
		if (!m_in.pending()) {
			m_in.reset();
		}
	}

}

void StreamLink::processWrite() {
	size_t got = Sock::bufferedSend(m_sock, m_out);

	if (got == (size_t) -1 && !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
		m_state = State::WRITE_ERROR;
		saved_errno = errno;
	}

	if (got > 0 && !m_out.pending()) {
		m_out.reset();
		onWriteCompleted(*this);
	}
}

