#include "circular_buffer.hpp"

#include <cstring>
#include <algorithm>

#define BYTE_OFFSET(buf, off) (((char *)buf)+off)

int CircularBuffer::add(const void *data, size_t sz) {
	size_t free = hasfree();
	if (!free) {
		return 0;
	} else if (sz > free) {
		sz = free;
	}

	size_t cont_len = cont_distance(end, begin, capacity);
	size_t done = std::min(cont_len, sz);
	memcpy(BYTE_OFFSET(buffer, end), data, done);

	if (done < sz) {
		size_t left = sz - done;
		memcpy(BYTE_OFFSET(buffer, 0), BYTE_OFFSET(data, done), left);
		done += left;
	}

	end += done;
	end %= capacity;
	size += done;

	return done;
}

int CircularBuffer::get(void *data, size_t sz) {
	size_t done = pick(data, sz);
	if (done) {
		begin += done;
		begin %= capacity;
		size -= done;
	}
	return done;
}

int CircularBuffer::pick(void *data, size_t sz) const {
	size_t filled = hasfilled();
	if (!filled) {
		return 0;
	} else if (sz > filled) {
		sz = filled;
	}

	size_t cont_len = cont_distance(begin, end, capacity);
	size_t done = std::min(cont_len, sz);
	memcpy(data, BYTE_OFFSET(buffer, begin), done);

	if (done < sz) {
		size_t left = sz - done;
		memcpy(BYTE_OFFSET(data, done), BYTE_OFFSET(buffer, 0), left);
		done += left;
	}

	return done;
}


size_t CircularBuffer::watch_in(void **data) const {
	size_t cont_dist = cont_distance(begin, end, capacity);
	if (cont_dist) {
		*data = BYTE_OFFSET(buffer, begin);
	}
	return cont_dist;
}

size_t CircularBuffer::watch_out(void **data) const {
	size_t cont_dist = cont_distance(end, begin, capacity);
	if (cont_dist) {
		*data = BYTE_OFFSET(buffer, end);
	}
	return cont_dist;
}

void CircularBuffer::flush_in(size_t sz) {
	if (!sz || sz >= size) {
		begin = end;
	} else {
		size_t cont = cont_distance(begin, end, capacity);
		if (sz > cont) {
			begin = sz - cont;
		} else {
			begin += sz;
		}
	}
}

void CircularBuffer::flush_out(size_t sz) {
	if (!sz || sz >= size) {
		begin = end;
	} else {
		size_t cont = cont_distance(begin, end, capacity);
		if (sz > cont) {
			begin = sz - cont;
		} else {
			begin += sz;
		}
	}
}

size_t CircularBuffer::distance(size_t from, size_t to, size_t size) {
	size_t result;
	if (from < to) {
		result = to - from;
	} else {
		result = size - (from + to);
	}
	return result;
}

size_t CircularBuffer::cont_distance(size_t from, size_t to, size_t size) {
	size_t result;
	if (from < to) {
		result = to - from;
	} else {
		result = size - from;
	}
	return result;
}
