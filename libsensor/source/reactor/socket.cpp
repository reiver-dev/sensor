#include "socket.hpp"

using namespace net;


Socket Socket::createFromText(const char *addr, const char *port, EndpointAddress *resolved) {
	struct addrinfo hints;
	struct addrinfo *result;
	int rc;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	rc = ::getaddrinfo(addr, port, &hints, &result);
	if (rc) {
		throw std::runtime_error("getaddrinfo failed");
	}
	std::unique_ptr<addrinfo, void(*)(addrinfo*)> info(result, freeaddrinfo);

	Socket fd = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);

	if (resolved)
		*resolved = EndpointAddress(info->ai_addr, info->ai_addrlen);

	return fd;
}

int Socket::set_nonblocking(bool value) {
	int flags;

	flags = fcntl(m_fd, F_GETFL, 0);
	if (flags == -1)
		flags = 0;
	if (value) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}

	return fcntl(m_fd, F_SETFL, flags);
}

Socket Socket::accept(EndpointAddress* address) {
	sockaddr_storage storage;
	socklen_t len;
	struct sockaddr *saddr = (struct sockaddr *)&storage;
	Socket::NATIVE fd = ::accept(m_fd, saddr, &len);
	if (fd != -1)
		*address = EndpointAddress(saddr, len);
	return fd;
}
