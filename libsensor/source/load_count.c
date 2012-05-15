#include <stdlib.h>
#include <string.h>

#include "arraylist.h"
#include "balancing.h"
#include "debug.h"
#include "hashmap.h"
#include "nodes.h"
#include "util.h"



static inline ArrayList get_client_moment_load(HashMap clientMomentLoads, struct Node *client) {
	return HashMap_get(clientMomentLoads, &client->ip4addr);
}

static int load_compare(const void *m1, const void *m2) {
	uint32_t first = (*(struct NodeLoad **)m1)->load;
	uint32_t second = (*(struct NodeLoad **)m2)->load;
	return first == second ? 0 : first > second ? 1 : -1;
}

static struct NodeLoad *load_create_item(ArrayList momentLoads) {

	struct NodeLoad *newLoad = malloc(sizeof(*newLoad));
	memset(newLoad, 0, sizeof(*newLoad));
	newLoad->timestamp = time(NULL);

	ArrayList_add(momentLoads, newLoad);

	return newLoad;
}

static struct NodeLoad *load_get_last(ArrayList momentLoads) {
	int loads = ArrayList_length(momentLoads);
	if (!loads) {
		return NULL;
	}
	return ArrayList_get(momentLoads, loads - 1);
}

static void load_close(ArrayList momentLoads, struct Node *client, uint32_t interval) {

	int loads = ArrayList_length(momentLoads);

	/* make copy of loads array */
	struct NodeLoad **data = malloc(loads * sizeof(struct NodeLoad *));
	memcpy(data, ArrayList_getData(momentLoads), loads * sizeof(struct NodeLoad *));

	qsort(data, loads, sizeof(void *), load_compare);

	int load;
	if (loads % 2) {
		int index = loads / 2;
		struct NodeLoad *median = data[index];
		load = median->load;
	} else {
		int index = loads / 2;
		struct NodeLoad *median = data[index];
		load = median->load;
		index++;
		median = data[index];
		load += median->load;
		load /= 2;
	}

	free(data);

	load /= interval;

	client->current_load = load;

}

void load_bytes_add(ArrayList momentLoads, size_t len) {
	struct NodeLoad *mload = load_get_last(momentLoads);
	if (!mload) {
		mload = load_create_item(momentLoads);
	}
	mload->load += len;
}


void load_count(HashMap clientMomentLoads, ArrayList owned, uint32_t load_interval, uint32_t load_count) {

	int len = ArrayList_length(owned);
	time_t now = time(NULL);

	for (int i = 0; i < len; i++) {
		struct Node *client = ArrayList_get(owned, i);
		ArrayList momentLoads = get_client_moment_load(clientMomentLoads, client);
		struct NodeLoad *lastLoad = load_get_last(momentLoads);

		if (!lastLoad) {
			continue;
		}

		int loads = ArrayList_length(momentLoads);

		uint32_t interval = now - lastLoad->timestamp;
		if (interval >= load_interval && loads >= load_count) {
			load_close(momentLoads, client, load_interval);
			ArrayList_remove_fast(momentLoads, 0);
		} else if (interval >= load_interval) {
			load_create_item(momentLoads);
		}

	}
}



