#include <stdlib.h>

#include "bestfit.h"
#include "nodes.h"
#include "arraylist.h"


static int roundload(int load) {
	int temp = load / 1000;
	return temp != 0 ? temp : 1;
}


static ArrayList init_solution(int sensor_count) {
	ArrayList solution = ArrayList_init(sensor_count, (ArrayList_destroyer)ArrayList_destroy);
	for (int i = 0; i < sensor_count; i++) {
		ArrayList owned_clients = ArrayList_init(0, 0);
		ArrayList_add(solution, owned_clients);
	}
	return solution;
}

static int client_load_compare_dec(const void *c1, const void *c2) {
	int load1 = (*(struct Node **) c1)->load;
	int load2 = (*(struct Node **) c2)->load;

	return load1 == load2 ? 0 : load1 < load2 ? 1 : -1;
}


static void sort_clients_by_load(struct Node **clients, size_t client_count) {
	qsort(clients, client_count, sizeof(struct Node *), client_load_compare_dec);
}

static int clients_load_sum(ArrayList clientsAL) {
	size_t clients_count = ArrayList_length(clientsAL);
	struct Node **clients = (struct Node **)ArrayList_getData(clientsAL);

	int result = 0;
	for (int i = 0; i < clients_count; i++) {
		uint32_t load = clients[i]->load;
		result += roundload(load);
	}
	return result;
}

static size_t find_lowest_load(ArrayList solution) {
	int sensor_count = ArrayList_length(solution);

	int loads[sensor_count];

	/* get load sums */
	for (size_t i = 0; i < sensor_count; i++) {
		ArrayList clients = ArrayList_get(solution, i);
		loads[i] = clients_load_sum(clients);
	}

	size_t index = -1;
	uint32_t min = UINT32_MAX;
	for (size_t i = 0; i < sensor_count; i++) {
		if (min > loads[i]) {
			min = loads[i];
			index = i;
		}
	}

	return index;
}

ArrayList bestfit_solution(struct Node **clients, size_t clients_count, size_t sensor_count) {
	ArrayList solution = init_solution(sensor_count);

	if (clients_count == 0) {
		goto End;
	}
	sort_clients_by_load(clients, clients_count);

	for (size_t i = 0; i < clients_count; i++) {
		size_t sack = find_lowest_load(solution);
		ArrayList sensor_owned = ArrayList_get(solution, sack);
		ArrayList_add(sensor_owned, clients[i]);
	}

End:

	return solution;
}
