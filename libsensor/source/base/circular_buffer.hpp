#ifndef CIRCULAR_BUFFER_HPP_
#define CIRCULAR_BUFFER_HPP_

#include <cstdlib>
#include <cstddef>

class CircularBuffer {
public:

	CircularBuffer(size_t sz)
		: begin(), end(), size(), capacity(sz) {
		buffer = malloc(sz);
	}

	~CircularBuffer() {
		free(buffer);
	}

	CircularBuffer(const CircularBuffer &o) = delete;
	CircularBuffer& operator==(const CircularBuffer &o) = delete;

	int add(const void *data, size_t sz);
	int get(void *data, size_t sz);
	int pick(void *data, size_t sz) const;
	size_t watch_in(void **data) const;
	size_t watch_out(void **data) const;
	void flush_in(size_t sz);
	void flush_out(size_t sz);

	size_t hasfilled() const {
		return begin != end ? distance(begin, end, capacity) : 0;
	}
	size_t hasfree() const {
		return begin != end ? distance(end, begin, capacity) : capacity;
	}

private:

	static size_t distance(size_t from, size_t to, size_t size);
	static size_t cont_distance(size_t from, size_t to, size_t size);

	size_t begin;
	size_t end;
	size_t size;

	const size_t capacity;
	void *buffer;
};




#endif /* CIRCULAR_BUFFER_HPP_ */
