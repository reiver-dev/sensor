#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <net/ethernet.h>
#include <net/if.h>

#define NODE_UNKNOWN 0
#define NODE_SENSOR 1
#define NODE_CLIENT 2

#define CLIENT_FREE 0
#define CLIENT_FOREIGN 1
#define CLIENT_MY 2


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

	union node_info info;
};

static struct Node *nodes;
static unsigned long node_count;


#define TWO(c)     (0x1u << (c))
#define MASK(c)    (((unsigned int)(-1)) / (TWO(TWO(c)) + 1u))
#define COUNT(x,c) ((x) & MASK(c)) + (((x) >> (TWO(c))) & MASK(c))
int bitcount(unsigned int n) {
	n = COUNT(n, 0);
	n = COUNT(n, 1);
	n = COUNT(n, 2);
	n = COUNT(n, 3);
	n = COUNT(n, 4);
	return n;
}
#undef COUNT
#undef MASK
#undef TWO

void balancing_initNodes(uint32_t ip4addr,uint32_t netmask) {

	unsigned long i;
	uint32_t network = ip4addr & netmask;

	node_count = (1 << (32 - bitcount(netmask))) - 2;

	nodes = malloc(node_count * sizeof(*nodes));
	memset(nodes, '\0', node_count);

	for (i = 0; i < node_count; i++) {
		struct Node *node = & nodes[i];
		node->ip4addr = network + (i+1);
		node->type    = NODE_UNKNOWN;
	}

}

void balancing_destroyNodes() {
	free(nodes);
	node_count = 0;
}

