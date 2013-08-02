/*
 * internet_address.cpp
 *
 *  Created on: 25.11.2012
 *      Author: reiver
 */

#include "internet_address.hpp"

InternetAddress::InternetAddress(struct in_addr addr) : family(AF_INET), address() {
	memcpy(&address.ip4, &addr, sizeof(addr));
}

InternetAddress::InternetAddress(struct in6_addr addr) : family(AF_INET6), address() {
	memcpy(&address.ip6, &addr, sizeof(addr));
}

InternetAddress::InternetAddress(struct sockaddr* addr, size_t len) {
	family = addr->sa_family;
	if (family == AF_INET && len >= sizeof(struct sockaddr_in)) {
		struct sockaddr_in *a = (struct sockaddr_in *)addr;
		address.ip4 = a->sin_addr;
	} else if (family == AF_INET6 && len >= sizeof(struct sockaddr_in6)) {
		struct sockaddr_in6 *a = (struct sockaddr_in6 *)addr;
		address.ip6 = a->sin6_addr;
	} else {
		family = AF_UNSPEC;
	}
}

size_t InternetAddress::get_sockaddr(struct sockaddr_storage *storage) {
	return 0;
}

InternetAddress::InternetAddress() : family() {
	//
}
