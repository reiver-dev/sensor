#ifndef INTERNET_ADDRESS_HPP_
#define INTERNET_ADDRESS_HPP_

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <cstring>

#include "../hash.hpp"

class InternetAddress {
public:

	InternetAddress();
	InternetAddress(struct in_addr  ip4);
	InternetAddress(struct in6_addr ip6);
	InternetAddress(struct sockaddr *addr, size_t len);


	~InternetAddress() {
		//
	}

	InternetAddress& operator=(const InternetAddress &other) {
		family = other.family;
		memcpy(&address, &other.address, sizeof(address));
		return *this;
	}


	void setAddr4(struct in_addr ip4);
	void setAddr6(struct in6_addr ip6);


	const in_addr& ip4() const {
		return address.ip4;
	}

	const in6_addr& ip6() const {
		return address.ip6;
	}

	bool operator == (const InternetAddress &other) const {
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
