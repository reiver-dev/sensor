#ifndef NODE_HPP_
#define NODE_HPP_

#include "networking.h"

struct NodeAddress {
	struct ether_addr hw;
	struct in_addr  in;
	struct in6_addr in6;
};


struct Node {
	NodeAddress addr;
	time_t last_check;
	int load;
	int current_load;
};


#endif /* NODE_HPP_ */
