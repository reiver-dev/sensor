#ifndef NODES_H_
#define NODES_H_

#include <stdbool.h>
#include <time.h>
#include <linux/if_ether.h>

#include "sensor_private.h"
#include "arraylist.h"

#define NODE_UNKNOWN 0
#define NODE_SENSOR 1
#define NODE_CLIENT 2

#define CLIENT_FREE 0
#define CLIENT_FOREIGN 1
#define CLIENT_OWNED 2


/* Types */

struct Node_sensor {
	char id[16];
	ArrayList clients;
};

struct Node_client {
	int type;
	int load;
};

union node_info {
	struct Node_client client;
	struct Node_sensor sensor;
};

struct Node {
	uint32_t ip4addr;
	uint8_t  hwaddr[ETH_ALEN];
	int      type;
	time_t   last_check;
	bool     is_online;

	union node_info info;
};

typedef void (*node_func)(struct Node *);

void nodes_init(struct CurrentAddress *curr);
void nodes_destroy();

int nodes_count();
struct Node *nodes_get();
struct Node *node_get(uint32_t ip);

ArrayList nodes_get_owned();
ArrayList nodes_get_sensors();

void node_answered(uint32_t ip4, uint8_t *hw);
void node_set_owned_by(struct Node *sensor, uint32_t ip4addr, int load);

void node_take(struct Node *node);

void nodes_sensors_foreach(node_func);
void nodes_neibours_foreach(node_func);
void nodes_owned_foreach(node_func);

#endif /* NODES_H_ */
