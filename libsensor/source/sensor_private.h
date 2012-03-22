#ifndef SENSOR_PRIVATE_H_
#define SENSOR_PRIVATE_H_

#include <stdbool.h>
#include <net/ethernet.h>
#include "queue.h"
#include "sensor.h"


enum sensor_error_e{
	SENSOR_SUCCESS,
	SENSOR_ALREADY_ACTIVATED,
	SENSOR_CREATE_SOCKET,
	SENSOR_BIND_SOCKET,
	SENSOR_IFACE_GET_FLAGS,
	SENSOR_IFACE_SET_FLAGS,
	SENSOR_IFACE_GET_INDEX
};


struct current{
	uint32_t ip4addr;
	uint32_t netmask;
	uint8_t hwaddr[ETH_ALEN];
	uint32_t gateway;
};

struct sensor {
	bool activated;
	int sock;
	struct current current;
	sensor_options_t opt;
	Queue_t captured;
	Queue_t dissected;
	sensor_dissect_f dissect_function;
	sensor_persist_f persist_function;
};


#endif /* SENSOR_PRIVATE_H_ */
