#ifndef INTERNET_ADDRESS_HPP_
#define INTERNET_ADDRESS_HPP_

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <cstring>
#include "debug.hpp"

class InternetAddress {
public:

	InternetAddress(struct in_addr  ip4);
	InternetAddress(struct in6_addr ip6);

	~InternetAddress() {
		//
	}

	size_t get_sockaddr(struct sockaddr_storage *storage);

	bool operator == (const InternetAddress &other) {
		bool result = false;
		if (family == other.family) {
			if (family == AF_INET) {
				result = memcmp(&ip4, &other.ip4, sizeof(ip4));
			} else {
				result = memcmp(&ip6, &other.ip6, sizeof(ip6));
			}
		}
		return result;
	}

	std::string to_string() {
		std::string result(INET6_ADDRSTRLEN, '\0');
		inet_ntop(family, (const void *)&address, &result.front(), result.length());
		return result;
	}

private:

	sa_family_t family;
	union {
		struct in_addr ip4;
		struct in6_addr ip6;
	} address;

	union {
		struct sockaddr_storage storage;
		struct sockaddr_in ip4;
		struct sockaddr_in6 ip6;
	};

};

#endif /* INTERNET_ADDRESS_HPP_ */
