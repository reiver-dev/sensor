/*
 * internet_address.cpp
 *
 *  Created on: 25.11.2012
 *      Author: reiver
 */

#include "internet_address.hpp"

InternetAddress::InternetAddress(struct in_addr addr) : family(AF_INET) {
	memset(&address, 0, sizeof(address));
	memcpy(&ip4, &addr, sizeof(addr));
}

InternetAddress::InternetAddress(struct in6_addr addr) : family(AF_INET6) {
	memset(&address, 0, sizeof(address));
	memcpy(&this->ip6, &addr, sizeof(addr));
}


size_t InternetAddress::get_sockaddr(struct sockaddr_storage *storage) {
	return 0;
}
