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

	void connect(char *address, char *port, const ConnectCB& continuation);
	void setConnectCallback(const ConnectCB& cb);

private:

	static void onEvent(EV_P_ ev_io *handler, int revent);

	typedef std::pair<EndpointAddress, ConnectCB> entry_t;

	struct hasher_t {
		size_t operator()(const ev_io &o) const {
			return static_cast<size_t>(o.fd);
		}
	};
	struct equals_t {
		bool operator()(const ev_io &o1, const ev_io &o2) const {
			return o1.fd == o2.fd && o1.cb == o2.cb;
		}
	};

	std::unordered_map<ev_io, entry_t, hasher_t, equals_t> m_pendingConnections;

	EventLoop *reactor;
};

}

#endif /* TCP_CONNECTOR_HPP_ */
