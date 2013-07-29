#include "address.hpp"

#include <cstring>
#include <stdexcept>

using namespace net;

static sockaddr_in &in4(const sockaddr_storage &address) {
	return *(sockaddr_in*) &address;
}

static sockaddr_in6 &in6(const sockaddr_storage &address) {
	return *(sockaddr_in6*) &address;
}

EndpointAddress::EndpointAddress(const EndpointAddress& o) {
	memcpy(this, &o, sizeof(*this));
}

EndpointAddress& EndpointAddress::operator=(
		const EndpointAddress& o) {
	memcpy(this, &o, sizeof(*this));
	return *this;
}

bool EndpointAddress::operator==(const EndpointAddress& o) const {
	return memcmp(this, &o, sizeof(*this)) == 0;
}

EndpointAddress::EndpointAddress(const in6_addr& addr, short port) {
	memset(&address, 0, sizeof(struct sockaddr_storage));
	in6(address).sin6_family = AF_INET6;
	in6(address).sin6_addr = addr;
	in6(address).sin6_port = htons(port);
}

EndpointAddress::EndpointAddress(const in_addr& addr, short port) {
	memset(&address, 0, sizeof(struct sockaddr_storage));
	in4(address).sin_family = AF_INET;
	in4(address).sin_addr = addr;
	in4(address).sin_port = htons(port);
}

EndpointAddress::EndpointAddress(struct sockaddr *addr, socklen_t len) {
	memset(&address, 0, sizeof(struct sockaddr_storage));
	if (addr->sa_family == AF_INET) {
		memcpy(&address, addr, sizeof(struct sockaddr_in));
	} else if (addr->sa_family == AF_INET6) {
		memcpy(&address, addr, sizeof(struct sockaddr_in6));
	} else {
		throw std::invalid_argument(
				"Only AF_INET or AF_INET6 addresses are supported");
	}
}

const sockaddr *EndpointAddress::get_sockaddr() const {
	return (struct sockaddr *) &address;
}

socklen_t EndpointAddress::get_socklen() const {
	switch (address.ss_family) {
	case AF_INET:
		return sizeof(sockaddr_in);
	case AF_INET6:
		return sizeof(sockaddr_in6);
	default:
		return 0;
	}
}

std::ostream& operator <<(std::ostream& s, const EndpointAddress& addr) {
	char address[INET6_ADDRSTRLEN + 1] = {0};
	uint16_t port = 0;
	switch(addr.get_sockaddr()->sa_family) {
	case AF_INET:
		inet_ntop(AF_INET, &((sockaddr_in *)addr.get_sockaddr())->sin_addr, address, INET6_ADDRSTRLEN);
		port = ntohs(((sockaddr_in *)addr.get_sockaddr())->sin_port);
		break;

	case AF_INET6:
		inet_ntop(AF_INET6, &((sockaddr_in6 *)addr.get_sockaddr())->sin6_addr, address, INET6_ADDRSTRLEN);
		port = ntohs(((sockaddr_in6 *)addr.get_sockaddr())->sin6_port);
		break;
	}
	s << address << ":" << port;
	return s;
}
