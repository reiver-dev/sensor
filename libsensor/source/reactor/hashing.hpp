#ifndef HASHING_HPP_
#define HASHING_HPP_

#include <functional>
#include <cstring>

#include <ev.h>

#include "address.hpp"
#include "stream_link.hpp"

namespace std {

template<>
struct hash<net::EndpointAddress> {
	size_t operator()(const net::EndpointAddress& o) const {
		size_t h = 0;
		size_t s = sizeof(net::EndpointAddress);
		char *data = (char *)&o;
		while(s) {
			h += 33 * h + *data++;
			s--;
		}
		return h;
	}
};

/*
template<>
struct hash<net::StreamLink> {
	size_t operator()(const net::StreamLink& o) const {
		hash<net::EndpointAddress> hash_fn;
		return hash_fn(o.remoteAddress());
	}
};
*/

}

#endif /* HASHING_HPP_ */
