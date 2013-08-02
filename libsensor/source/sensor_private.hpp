#ifndef SENSOR_PRIVATE_H_
#define SENSOR_PRIVATE_H_

#include <stdbool.h>
#include "sensor.h"
#include "net/netinfo.h"


enum sensor_error_e {
	SENSOR_SUCCESS,
	SENSOR_ALREADY_ACTIVATED,
	SENSOR_CREATE_SOCKET,
	SENSOR_BIND_SOCKET,
	SENSOR_IFACE_GET_FLAGS,
	SENSOR_IFACE_SET_FLAGS,
	SENSOR_IFACE_GET_INDEX
};

struct sensor {
	bool activated;
	int sock;
	struct InterfaceInfo captureInterface;
	sensor_options_t opt;
	sensor_persist_f persist_function;
};

void sensor_log_packet(int size);
int sensor_notify();

#endif /* SENSOR_PRIVATE_H_ */
