#ifndef HASH_HPP_
#define HASH_HPP_

#include "net/socket_utils.h"

namespace hashing {

typedef size_t hash_t;

inline hash_t times33(const char *data, size_t size) {
	hash_t hash = 0;
	while (size) {
		hash = 33 * hash + *data++;
		size--;
	}
	return hash;
}

inline hash_t sdbm(const char *data, size_t size) {
	hash_t hash = 0;
	while (size) {
		hash = *data++ + (hash << 6) + (hash << 16) - hash;
		size--;
	}
	return hash;
}


template<typename T>
hash_t times33(const T &t) {
	char *data = (char *) &t;
	size_t size = sizeof(T);
	return times33(data, size);
}

template<typename T>
hash_t sdbm(const T &t) {
	char *data = (char *) &t;
	size_t size = sizeof(T);
	return sdbm(data, size);
}

}

#endif /* HASH_HPP_ */
