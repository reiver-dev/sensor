#ifndef ADDRESS_HPP_
#define ADDRESS_HPP_

#include <string.h>

#include "networking.h"
#include "hash.hpp"

#define SOCK_ADDR(type, storage) *((struct type *) &storage)

class Address {
public:

	Address() : storage() {
		//
	}

	Address(struct sockaddr_storage sa) : storage(sa) {
		//
	}

	Address(struct sockaddr_in sa) : storage() {
		SOCK_ADDR(sockaddr_in, storage) = sa;
	}

	Address(struct sockaddr_in6 sa) : storage() {
		SOCK_ADDR(sockaddr_in6, storage) = sa;
	}

	bool operator== (const Address &a) {
		return memcmp(this, &a, sizeof(Address)) == 0;
	}

private:
	struct sockaddr_storage storage;
};

namespace std {
template<>
struct hash<Address> {
	size_t operator()(const Address &a) {
		return hashing::times33(a);
	}
};
}

#endif /* ADDRESS_HPP_ */
