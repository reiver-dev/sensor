#ifndef SOCKET_BUFFER_HPP_
#define SOCKET_BUFFER_HPP_

#include "base/circular_buffer.hpp"

class SocketBuffer: public CircularBuffer {
public:
	int send(int fd);
	int recv(int fd);
};

#endif /* SOCKET_BUFFER_HPP_ */
