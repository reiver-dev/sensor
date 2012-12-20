#include "sockbuffer.hpp"

#include <cstring>
#include <algorithm>

int SockBuffer::send(int fd) {
	return 0;
}

int SockBuffer::recv(int fd) {
	return 0;
}

int SockBuffer::add(const void *data, size_t sz) {
	if (begin == end && size != 0)
		return 0;

	size_t from = end;
	size_t to   = begin == 0 ? size : begin;

	int done = move_data(from, to, data, buffer, sz);
	if (done > 0) {
		end += done;
		size += done;
	}
	return done;
}

int SockBuffer::get(void *data, size_t sz) {
	if (begin == end && size == 0)
		return 0;

	size_t from = begin;
	size_t to   = end == 0 ? size : end;

	int done = move_data(from, to, buffer, data, sz);
	if (done > 0) {
		begin += done;
		size -= done;
	}
	return done;
}

int SockBuffer::pick(void *data, size_t sz) {
	return move_data(begin, end, buffer, data, sz);
}


size_t SockBuffer::distance(size_t from, size_t to) {
	ssize_t result;
	if (from < to) {
		result = to - from;
	} else {
		result = capacity - from + to;
	}
	return result;
}

size_t SockBuffer::cont_distance(size_t from, size_t to) {
	ssize_t result;
	if (from < to) {
		result = to - from;
	} else {
		result = capacity - from;
	}
	return result;
}

int SockBuffer::move_data(size_t begin, size_t end, const void* from, void* to, size_t sz) {
	size_t done = 0;

	while (sz - done) {
		size_t cont_len = cont_distance(begin, end);
		size_t to_copy = std::max(cont_len, sz - done);
		size_t newpos = begin + to_copy;

		// if passed full circle
		if (!distance(newpos, end)) {
			break;
		}

		// overlap
		newpos %= capacity;

		memcpy(((char *)to) + done, ((char *)from) + begin, to_copy);

		begin = newpos;
		done += to_copy;
	}

	return done;
}
