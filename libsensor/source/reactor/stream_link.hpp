#ifndef STREAM_LINK_HPP_
#define STREAM_LINK_HPP_

#include "event_loop.hpp"
#include "socket.hpp"
#include "callback.hpp"
#include "buffer.hpp"

namespace net {

class StreamLink {

	enum class State {
		OK, DISCONNECT, READ_ERROR, WRITE_ERROR
	};

public:

	typedef CB::Callback<void(StreamLink&)> Callback;

	StreamLink(const EndpointAddress& address) :
			m_state(State::OK),
			saved_errno(0),
			m_receive(false),
			m_address(address),
			m_reactor(nullptr) {
	}
	~StreamLink();


	bool isSending() const {
		return m_out.pending() != 0;
	}

	bool isReceiving() const {
		return m_receive;
	}

	const EndpointAddress& remoteAddress() const {
		return m_address;
	}

	void setReadCallback(const Callback& cb);
	void setWriteCallback(const Callback& cb);
	void setDisposeCallback(const Callback& cb);

	void setReceiving(bool val) {
		if (m_state == State::OK)
			m_receive = val;
	}

	int errorOccured() {
		if (m_state == State::READ_ERROR || m_state == State::WRITE_ERROR) {
			return saved_errno;
		}
		return 0;
	}

	void makeDisconnect() {
		if (m_state == State::OK)
			m_state = State::DISCONNECT;
	}

	void peek(char **d, size_t *l);
	void advance(size_t l);
	void write(char *data, size_t len);

private:

	static void eventCallback(EV_P_ ev_io *handler, int revents);

	void initialize(Socket sock, EventLoop *reactor);
	void initialize(const ev_io& handler, EventLoop *reactor);

	void processRead();
	void processWrite();
	bool prepareEvents();

	ev_io m_handler;

	State m_state;
	int saved_errno;
	bool m_receive;

	Socket m_sock;
	EndpointAddress m_address;

	Callback onRead;
	Callback onWriteCompleted;
	Callback onDispose;

	Buffer m_in;
	Buffer m_out;

	EventLoop *m_reactor;

	friend class TcpConnector;
	friend class TcpAcceptor;

};

}

#endif /* STREAM_LINK_HPP_ */
