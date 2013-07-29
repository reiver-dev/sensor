#ifndef ADDRESS_HPP_
#define ADDRESS_HPP_

#include <iostream>
#include "net_system.h"

namespace net {

class EndpointAddress {
public:

	EndpointAddress() {
		//
	}

	EndpointAddress(const EndpointAddress& o);
	EndpointAddress& operator=(const EndpointAddress& o);

	bool operator== (const EndpointAddress& o) const;

	EndpointAddress(const in6_addr& addr, short port);
	EndpointAddress(const in_addr& addr, short port);
	EndpointAddress(struct sockaddr *addr, socklen_t len);

	const sockaddr *get_sockaddr() const;
	socklen_t get_socklen() const;

private:
	sockaddr_storage address;
};

}

std::ostream& operator <<(std::ostream& s, const net::EndpointAddress& addr);


#endif /* ADDRESS_HPP_ */
