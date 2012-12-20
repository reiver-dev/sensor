#ifndef SOCKBUFFER_HPP_
#define SOCKBUFFER_HPP_

#include <cstdlib>
#include <cstddef>

class SockBuffer {
public:

	SockBuffer(size_t sz)
		: begin(), end(), size(), capacity(sz) {
		buffer = malloc(sz);
	}

	~SockBuffer() {
		free(buffer);
	}

	SockBuffer(const SockBuffer &o) = delete;
	SockBuffer& operator==(const SockBuffer &o) = delete;

	int send(int fd);
	int recv(int fd);

	int add(const void *data, size_t sz);
	int get(void *data, size_t sz);
	int pick(void *data, size_t sz) const;

	size_t hasfilled() {
		return begin != end ? distance(begin, end) : 0;
	}
	size_t hasfree() {
		return begin != end ? distance(end, begin) : size;
	}

private:

	size_t distance(size_t from, size_t to) const;
	size_t cont_distance(size_t from, size_t to) const;
	int move_data(size_t begin, size_t end, const void *from, void *to, size_t sz) const;

	size_t begin;
	size_t end;
	size_t size;

	const size_t capacity;
	void *buffer;
};




#endif /* SOCKBUFFER_HPP_ */
