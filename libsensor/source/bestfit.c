#include <stdlib.h>

#include "bestfit.h"
#include "nodes.h"
#include "arraylist.h"


static int roundload(int load) {
	return load / 1000;
}


static ArrayList init_solution(int sensor_count) {
	ArrayList solution = ArrayList_init(sensor_count, (ArrayList_destroyer)ArrayList_destroy);
	for (int i = 0; i < sensor_count; i++) {
		ArrayList owned_clients = ArrayList_init(0, 0);
		ArrayList_add(solution, owned_clients);
	}
	return solution;
}


static ArrayList get_all_clients() {
	struct Node *allNodes = nodes_get();
	size_t nodesCount = nodes_count();
	ArrayList clients = ArrayList_init(0,0);
	for (size_t i = 0; i < nodesCount; i++) {
		if (allNodes[i].type == NODE_TYPE_CLIENT && allNodes[i].is_online) {
			ArrayList_add(clients, &allNodes[i]);
		}
	}
	return clients;
}

static int client_load_compare_dec(const void *c1, const void *c2) {
	int load1 = (*(struct Node **) c1)->info.client.load.load;
	int load2 = (*(struct Node **) c2)->info.client.load.load;

	return load1 == load2 ? 0 : load1 < load2 ? 1 : -1;
}


static void sort_clients_by_load(ArrayList clients) {
	struct Node **cl = (struct Node **)ArrayList_getData(clients);
	qsort(cl, ArrayList_length(clients), sizeof(struct Node *), client_load_compare_dec);
}

static int clients_load_sum(ArrayList clientsAL) {
	size_t clients_count = ArrayList_length(clientsAL);
	struct Node **clients = (struct Node **)ArrayList_getData(clientsAL);

	int result = 0;
	for (int i = 0; i < clients_count; i++) {
		int load = clients[i]->info.client.load.load;
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
	int min = -1;
	for (size_t i = 0; i < sensor_count; i++) {
		if (min < loads[i]) {
			min = loads[i];
			index = i;
		}
	}

	return index;
}

ArrayList bestfit_solution(ArrayList sensors) {
	int sensor_count = ArrayList_length(sensors);

	ArrayList solution = init_solution(sensor_count);

	ArrayList allClients = get_all_clients();
	size_t clients_count = ArrayList_length(allClients);

	if (clients_count == 0) {
		goto End;
	}
	sort_clients_by_load(allClients);

	for (size_t i = 0; i < clients_count; i++) {
		size_t sack = find_lowest_load(solution);
		ArrayList sensor_owned = ArrayList_get(solution, sack);
		ArrayList_add(sensor_owned, ArrayList_get(allClients, i));
	}

End:

	free(allClients);
	return solution;
}
