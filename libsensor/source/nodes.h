#ifndef NODES_H_
#define NODES_H_

#include <stdbool.h>
#include <time.h>
#include <linux/if_ether.h>

#include "sensor_private.h"
#include "arraylist.h"


#define NODE_LOAD_NOT_READY UINT32_MAX

/* Types */
struct NodeLoad {
	time_t timestamp;
	uint32_t load;
};

struct Node {
	struct NetAddress addr;

	time_t   last_check;
	bool     is_online;

	uint32_t load;
	uint32_t current_load;
	struct Node *owned_by;
};

typedef void (*node_func)(struct Node *);

void nodes_init(struct InterfaceInfo *curr);
void nodes_destroy();

size_t nodes_count();
struct Node **nodes_get();
void node_answered(uint32_t ip4, uint8_t *hw);

struct Node *nodes_get_node(uint32_t ip);
void nodes_remove(uint32_t ip);
struct Node *nodes_get_destination(uint32_t ip);
bool nodes_is_my_addr(uint32_t ip4);
bool nodes_is_me(struct Node *node);
struct Node *nodes_get_me();
struct Node *nodes_get_gateway();

#endif /* NODES_H_ */
