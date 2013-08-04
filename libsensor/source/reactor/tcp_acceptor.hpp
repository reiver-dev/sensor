#ifndef TCP_ACCEPTOR_HPP_
#define TCP_ACCEPTOR_HPP_

#include "stream_link.hpp"
#include "event_loop.hpp"
#include "callback.hpp"

namespace net {

class TcpAcceptor {
public:

	typedef CB::Callback<StreamLink* (const EndpointAddress&)> AcceptCB;

	TcpAcceptor(const char *addr, const char *port);
	void setAcceptCallback(const AcceptCB& cb);

	void start(EventLoop *r);
	void stop();

private:

	static void onEvent(EV_P_ ev_io *handler, int revent);

	ev_io handler;
	AcceptCB onAccept;

	EndpointAddress m_address;
	Socket sock;

	EventLoop *reactor;
};

}

#endif /* TCP_ACCEPTOR_HPP_ */
