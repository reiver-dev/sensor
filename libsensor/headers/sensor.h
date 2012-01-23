#ifndef SENSOR_H
#define SENSOR_H
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <net/ethernet.h>
#include <net/if.h>
#include "queue.h"

enum sensor_error_e{
	SENSOR_SUCCESS,
	SENSOR_ALREADY_ACTIVATED,
	SENSOR_CREATE_SOCKET,
	SENSOR_BIND_SOCKET,
	SENSOR_IFACE_GET_FLAGS,
	SENSOR_IFACE_SET_FLAGS,
	SENSOR_IFACE_GET_INDEX
};


typedef int (*sensor_persist_f)(Queue_t *in);

typedef int (*sensor_dissect_f)(Queue_t *in, Queue_t *out);

typedef struct {
	char device_name[IF_NAMESIZE];
	bool promiscuous;
	uint32_t buffersize;
	uint32_t capture_timeout;
	uint32_t dissect_timeout;
	uint32_t persist_timeout;
	bool enable_redirect;
} sensor_options_t;

struct sensor{
	bool activated;
	int sock;
	uint8_t hwaddr[ETH_ALEN];
	uint32_t ip4addr;
	sensor_options_t opt;
	Queue_t captured;
	Queue_t dissected;
	sensor_dissect_f dissect_function;
	sensor_persist_f persist_function;
};

typedef struct sensor sensor_t;


typedef struct sensor_captured_s{
	time_t timestamp;
	int length;
	uint8_t* buffer;
} sensor_captured_t;

typedef struct sensor_dissected_s{
	time_t timestamp;
	int content_length;
	int payload_length;
	char mac_source[18];
	char mac_dest[18];
	char* content;
	uint8_t* payload;
} sensor_dissected_t;


sensor_captured_t* init_captured(uint8_t *buffer, int len);
void destroy_captured(sensor_captured_t *captured);
void destroy_dissected(sensor_dissected_t *dissected);



sensor_t sensor_init();
int sensor_set_options(sensor_t *config, sensor_options_t options);
int sensor_set_dissection_default(sensor_t *config);
int sensor_set_persist_callback(sensor_t *config, sensor_persist_f callback);
int sensor_loop(sensor_t *config);
void sensor_breakloop(sensor_t *config);


#endif /*SENSOR_H*/
