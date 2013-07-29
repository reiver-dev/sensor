#ifndef BUFFER_HPP_
#define BUFFER_HPP_

#include <cstdlib>
#include <cstring>
#include <vector>

namespace net {

class Buffer {
public:

	Buffer() : Buffer(2048) {

	}

	Buffer(size_t initial) {
		r = w = begin = (char *)malloc(initial);
		end = begin + initial;
		memset(begin, 0, size());
	}

	char* readPtr() {
		return r;
	}

	char* writePtr() {
		return w;
	}

	size_t pending() const {
		return w - r;
	}
	size_t writable() const {
		return end - w;
	}

	void processed(size_t bytes) {
		r += bytes;
	}

	void written(size_t bytes) {
		w += bytes;
	}

	void append(char *data, size_t len) {
		if (writable() < len)
			expandTo(size() + len);
		memcpy(w, data, len);
		written(len);
	}

	void reset() {
		r = w = begin;
	}

private:

	size_t size() const {
		return end - begin;
	}

	void expandTo(size_t s) {
		size_t readPos = r - begin;
		size_t writePos = w - begin;
		begin = (char *)realloc(begin, s);
		r = begin + readPos;
		w = begin + writePos;
		end = begin + s;
		memset(w, 0, writable());
	}

	char* begin;
	char* end;
	char* r;
	char* w;

};



}

#endif /* BUFFER_HPP_ */
