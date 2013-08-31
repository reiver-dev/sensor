#ifndef NODE_HPP_
#define NODE_HPP_

#include "base/internet_address.hpp"
#include "base/hw_address.hpp"

struct NodeAddress {
	HwAddress hw;
	InternetAddress in;
};


struct Node {
	NodeAddress addr;
	time_t last_check;
	int load;
	int moment_load;
};


#endif /* NODE_HPP_ */
