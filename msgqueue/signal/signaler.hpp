#ifndef SIGNALER_HPP_
#define SIGNALER_HPP_

#include <assert.h>
#include <unistd.h>
#include <sys/eventfd.h>

class Signaler {
public:

	typedef int fd_t;

	Signaler() : wr(create_fd()), rd(wr) {

	}

	int send() {
		const uint64_t inc = 1;
		ssize_t sz = write(wr, &inc, sizeof(inc));
		return sz;
	}

	int recv() {
		uint64_t ret;
		ssize_t sz = read(rd, &ret, sizeof (ret));
		assert(sz == sizeof(ret));
		return ret;
	}


	fd_t write_fd() {
		return wr;
	}

	fd_t read_fd() {
		return rd;
	}

private:

	static fd_t create_fd() {
		fd_t fd = eventfd(0, 0);
		set_nonblocking(fd);
		return fd;
	}

	fd_t wr;
	fd_t rd;
};

#endif /* SIGNALER_HPP_ */
