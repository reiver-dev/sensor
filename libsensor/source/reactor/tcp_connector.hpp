#ifndef TCP_CONNECTOR_HPP_
#define TCP_CONNECTOR_HPP_

#include <unordered_map>

#include "stream_link.hpp"
#include "event_loop.hpp"
#include "callback.hpp"


namespace net {

class TcpConnector {
public:

	typedef CB::Callback<StreamLink* (const EndpointAddress&)> ConnectCB;

	void connect(EventLoop *loop, char *address, char *port, const ConnectCB& continuation);
	void setConnectCallback(const ConnectCB& cb);

private:

	static void onEvent(EV_P_ ev_io *handler, int revent);

	struct entry_t {
		ev_io handler;
		EndpointAddress addr;
		ConnectCB cb;
	};

	std::unordered_map<Socket::NATIVE, entry_t> m_pendingConnections;

	EventLoop *reactor;
};

}

#endif /* TCP_CONNECTOR_HPP_ */
