#include "socket_buffer.hpp"

#include <sys/socket.h>
#include <errno.h>

int SocketBuffer::send(int fd) {
	void *data;
	size_t sz = watch_in(&data);
	if (sz < 1) {
		return -1;
	}

	ssize_t done = ::send(fd, data, sz, 0);
	if (done == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
			return 0;
		}
	}

//	flush(done);

	return done;
}

int SocketBuffer::recv(int fd) {
	void *data;
	size_t sz = watch_out(&data);
	if (sz < 1) {
		return -1;
	}
	ssize_t done = ::recv(fd, data, sz, 0);

	if (done == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
			return 0;
		}
	}

	if (done == 0) {
		return -1;
	}

	return done;
}
