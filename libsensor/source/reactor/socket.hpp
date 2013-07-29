#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <stdexcept>
#include <memory>

#include <netdb.h>
#include <fcntl.h>

#include "net_system.h"
#include "address.hpp"
#include "buffer.hpp"

namespace net {

class Socket {
public:

	typedef int NATIVE;
	typedef iovec IOVEC;

	Socket() : m_fd(0) {

	}

	Socket(NATIVE sock) : m_fd(sock) {

	}

	static Socket createFromText(const char *addr, const char *port, EndpointAddress *resolved);

	int set_nonblocking(bool value);

	int connect(const EndpointAddress& address) {
		return ::connect(m_fd, address.get_sockaddr(), address.get_socklen());
	}

	int listen() {
		return ::listen(m_fd, 99);
	}

	Socket accept(EndpointAddress *address);

	int setReuseAddr(bool value) noexcept {
		int val = value ? 1 : 0;
		return ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	}

	int bind(const EndpointAddress& address) noexcept {
		return ::bind(m_fd, address.get_sockaddr(), address.get_socklen());
	}

	int close() {
		return ::close(m_fd);
	}

	ssize_t readv(IOVEC *vectors, int count) {
		return ::readv(m_fd, vectors, count);
	}

	ssize_t write(void *buffer, size_t length) {
		return ::send(m_fd, buffer, length, 0);
	}

	Socket::NATIVE fd() {
		return m_fd;
	}

	bool operator!() const {
		return (m_fd == -1);
	}

private:
	NATIVE m_fd;
};




}

#endif /* SOCKET_HPP_ */
