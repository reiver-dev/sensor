#ifndef NODES_H_
#define NODES_H_

#include <stdbool.h>
#include <time.h>
#include <linux/if_ether.h>

#include "sensor_private.h"
#include "arraylist.h"


#define NODE_TYPE_OFFLINE 0
#define NODE_TYPE_SENSOR 1
#define NODE_TYPE_CLIENT 2
#define NODE_TYPE_GATEWAY 3


/* Types */
struct NodeLoad {
	time_t timestamp;
	uint32_t load;
};

struct Node {
	uint32_t ip4addr;
	uint8_t  hwaddr[ETH_ALEN];

	time_t   last_check;
	bool     is_online;

	uint32_t load;
	struct Node *owned_by;
};

typedef void (*node_func)(struct Node *);

void nodes_init(struct CurrentAddress *curr);
void nodes_destroy();

size_t nodes_count();
struct Node **nodes_get();
void node_answered(uint32_t ip4, uint8_t *hw);

struct Node *nodes_get_node(uint32_t ip);
struct Node *nodes_get_destination(uint32_t ip);
bool nodes_is_my_addr(uint32_t ip4);
bool nodes_is_me(struct Node *node);
struct Node *nodes_get_me();
struct Node *nodes_get_gateway();

#endif /* NODES_H_ */
