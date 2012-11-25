#ifndef NODES_H_
#define NODES_H_

#include <stdbool.h>
#include <time.h>
#include <linux/if_ether.h>

#include "sensor_private.hpp"


#define NODE_LOAD_NOT_READY UINT32_MAX

/* Types */
struct NodeLoad {
	time_t timestamp;
	uint32_t load;
};

struct Node {
	struct NodeAddress addr;

	time_t   last_check;
	bool     is_online;

	uint32_t load;
	uint32_t current_load;
	struct Node *owned_by;
};

#endif /* NODES_H_ */
