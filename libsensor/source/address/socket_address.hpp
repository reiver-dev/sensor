#ifndef ADDRESS_HPP_
#define ADDRESS_HPP_

#include <string.h>

#include "networking.h"
#include "hash.hpp"

#define SOCK_ADDR(type, storage) *((struct type *) &storage)

class SocketAddress {
public:

	SocketAddress() : storage() {
		//
	}

	SocketAddress(struct sockaddr_storage sa) : storage(sa) {
		//
	}

	SocketAddress(struct sockaddr_in sa) : storage() {
		SOCK_ADDR(sockaddr_in, storage) = sa;
	}

	SocketAddress(struct sockaddr_in6 sa) : storage() {
		SOCK_ADDR(sockaddr_in6, storage) = sa;
	}

	bool operator== (const SocketAddress &a) {
		return memcmp(this, &a, sizeof(SocketAddress)) == 0;
	}

private:
	struct sockaddr_storage storage;
};

namespace std {
template<>
struct hash<SocketAddress> {
	size_t operator()(const SocketAddress &a) {
		return hashing::times33(a);
	}
};
}

#endif /* ADDRESS_HPP_ */
