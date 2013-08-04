#ifndef INTERNET_ADDRESS_HPP_
#define INTERNET_ADDRESS_HPP_

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <cstring>

#include "hash.hpp"

class InternetAddress {
public:

	InternetAddress();
	InternetAddress(struct in_addr  ip4);
	InternetAddress(struct in6_addr ip6);
	InternetAddress(struct sockaddr *addr, size_t len);

	InternetAddress& operator=(const InternetAddress &other);

	void setAddr4(struct in_addr ip4);
	void setAddr6(const struct in6_addr &ip6);

	bool operator == (const InternetAddress &other) const;

	char *toString(char buffer[INET6_ADDRSTRLEN]) const;

	const in_addr& ip4() const {
		return address.ip4;
	}

	const in6_addr& ip6() const {
		return address.ip6;
	}

private:

	sa_family_t family;
	union {
		struct in_addr ip4;
		struct in6_addr ip6;
	} address;


};

namespace std {
template<>
struct hash<InternetAddress> {
	size_t operator()(const InternetAddress &a) const {
		return hashing::times33(a);
	}
};
}

#endif /* INTERNET_ADDRESS_HPP_ */
