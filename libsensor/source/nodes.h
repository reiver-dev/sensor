#ifndef NODES_H_
#define NODES_H_

#include <stdbool.h>
#include <time.h>
#include <linux/if_ether.h>

#include "sensor_private.h"
#include "arraylist.h"

#define NODE_TYPE_UNKNOWN 0
#define NODE_TYPE_SENSOR 1
#define NODE_TYPE_CLIENT 2
#define NODE_TYPE_GATEWAY 3

#define NODE_CLIENT_FREE 0
#define NODE_CLIENT_FOREIGN 1
#define NODE_CLIENT_OWNED 2


/* Types */

struct Node_sensor {
	ArrayList clients;
};

struct NodeLoad {
	time_t timestamp;
	uint32_t load;
};

struct Node_client {
	struct Node *owned_by;
	struct NodeLoad load;
	ArrayList moment_load;
};

union node_info {
	struct Node_client client;
	struct Node_sensor sensor;
};

struct Node {
	uint32_t ip4addr;
	uint8_t  hwaddr[ETH_ALEN];

	time_t   last_check;
	bool     is_online;

	uint32_t type;
	uint32_t load;
	struct Node *owned_by;

};

typedef void (*node_func)(struct Node *);

void nodes_init(struct CurrentAddress *curr);
void nodes_destroy();

int nodes_count();
struct Node *nodes_get();
struct Node *node_get(uint32_t ip);
struct Node *node_get_destination(uint32_t ip);
struct Node *node_get_gateway();
struct Node *node_get_me();

bool node_is_me(struct Node *node);

void node_set_sensor(struct Node *node);
void node_set_client(struct Node *node);
void node_set_gateway(struct Node *node);

ArrayList nodes_get_sensors();

void node_answered(uint32_t ip4, uint8_t *hw);

char *node_toString(struct Node *node);

#endif /* NODES_H_ */
