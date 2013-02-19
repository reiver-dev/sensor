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

InternetAddress InternetAddress::fromSocketAddress(struct sockaddr* addr) {
	InternetAddress internetAddress;
	internetAddress.family = addr->sa_family;
	if (internetAddress.family == AF_INET) {
		struct sockaddr_in *a = (struct sockaddr_in *)addr;
		internetAddress.address.ip4 = a->sin_addr;
	} else {
		struct sockaddr_in6 *a = (struct sockaddr_in6 *)addr;
		internetAddress.address.ip6 = a->sin6_addr;
	}
	return internetAddress;
}

size_t InternetAddress::get_sockaddr(struct sockaddr_storage *storage) {
	return 0;
}

InternetAddress::InternetAddress() : family() {
	//
}
