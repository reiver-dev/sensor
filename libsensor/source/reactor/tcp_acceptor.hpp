#ifndef TCP_ACCEPTOR_HPP_
#define TCP_ACCEPTOR_HPP_

#include <stdio.h>

#include "stream_link.hpp"
#include "event_loop.hpp"
#include "callback.hpp"

namespace net {

class TcpAcceptor {
public:

	typedef CB::Callback<StreamLink* (const EndpointAddress&)> AcceptCB;

	void init(const char *addr, const char *port, const AcceptCB& cb);
	void init(const char* addr, uint16_t port, const AcceptCB& cb);

	void start(EventLoop *r);
	void stop();

private:

	static void onEvent(EV_P_ ev_io *handler, int revent);

	ev_io handler;
	AcceptCB onAccept;

	EndpointAddress m_address;
	SocketMain sock;

	EventLoop *reactor;
};



}

#endif /* TCP_ACCEPTOR_HPP_ */
