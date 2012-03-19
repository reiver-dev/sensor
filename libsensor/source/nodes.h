#ifndef NODES_H_
#define NODES_H_

#include <stdbool.h>
#include <time.h>
#include <linux/if_ether.h>

#define NODE_UNKNOWN 0
#define NODE_SENSOR 1
#define NODE_CLIENT 2

#define CLIENT_FREE 0
#define CLIENT_FOREIGN 1
#define CLIENT_OWNED 2


/* Types */
struct Node_sensor {
	char id[16];
	int clients_count;
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

void nodes_init(const uint32_t ip4addr, const uint32_t netmask);
int nodes_count();
struct Node *nodes_get();
struct Node *node_get(uint32_t ip);
void nodes_destroy();

void node_answered(uint32_t ip4, uint8_t *hw);

#endif /* NODES_H_ */
