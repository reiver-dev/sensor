#include "utils.hpp"

using namespace net;

size_t Sock::bufferedRecv(Socket sock, Buffer &buffer) {
	char extended[4096] = { 0 };
	Socket::IOVEC vectors[2];
	ssize_t writable = buffer.writable();
	vectors[0].iov_base = buffer.writePtr();
	vectors[0].iov_len = writable;
	vectors[1].iov_base = extended;
	vectors[1].iov_len = 4096;
	ssize_t got = sock.readv(vectors, 2);
	if (got > writable) {
		buffer.written(got);
		buffer.append(extended, got - writable);
	} else if (got > 0) {
		buffer.written(got);
	}
	return got;
}

size_t Sock::bufferedSend(Socket sock, Buffer &buffer) {
	char *buf = buffer.readPtr();
	size_t len = buffer.pending();
	ssize_t got = sock.write(buf, len);
	if (got > 0) {
		buffer.processed(got);
	}
	return got;
}

