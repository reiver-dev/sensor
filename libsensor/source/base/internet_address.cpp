/*
 * internet_address.cpp
 *
 *  Created on: 25.11.2012
 *      Author: reiver
 */

#include "internet_address.hpp"

InternetAddress::InternetAddress() : family(AF_UNSPEC), address() {
	//
}

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

void InternetAddress::setAddr4(struct in_addr ip4) {
	family = AF_INET;
	address.ip4 = ip4;
}
void InternetAddress::setAddr6(const struct in6_addr &ip6) {
	family = AF_INET6;
	address.ip6 = ip6;
}

InternetAddress& InternetAddress::operator=(const InternetAddress &other) {
	family = other.family;
	memcpy(&address, &other.address, sizeof(address));
	return *this;
}

bool InternetAddress::operator==(const InternetAddress &other) const {
	bool result = false;
	if (family == other.family) {
		if (family == AF_INET) {
			result = memcmp(&address.ip4, &other.address.ip4, sizeof(address.ip4));
		} else {
			result = memcmp(&address.ip6, &other.address.ip6, sizeof(address.ip6));
		}
	}
	return result;
}

char *InternetAddress::toString(char buffer[INET6_ADDRSTRLEN]) const {
	inet_ntop(family, &address, buffer, INET6_ADDRSTRLEN);
	return buffer;
}
