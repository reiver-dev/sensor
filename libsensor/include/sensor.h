#ifndef SENSOR_H
#define SENSOR_H
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <net/if.h>
#include <queue.h>


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



typedef sensor_dissected_t *(*sensor_dissect_f)(sensor_captured_t *captured);
typedef int (*sensor_persist_f)(Queue_t in);


/* Options */
typedef struct {
	bool promiscuous;
	uint32_t buffersize;
	uint32_t timeout;
} sensor_opt_capture;

typedef struct {
	uint32_t timeout;
} sensor_opt_dissect;

typedef struct {
	uint32_t timeout;
} sensor_opt_persist;

typedef struct {
	uint32_t timeout;
	uint32_t initial_timeout;
	uint32_t node_disconnect_timeout;
} sensor_opt_survey;

typedef struct {
	uint32_t timeout;
	uint32_t initial_timeout;
	uint32_t info_timeout;
	uint32_t session_timeout;
	uint32_t modify_timeout;
	uint32_t load_count;
	uint32_t load_interval;
	bool enable_redirect;
	bool enable_modify;
} sensor_opt_balancing;

typedef struct {
	char device_name[IF_NAMESIZE];
	sensor_opt_capture   capture;
	sensor_opt_dissect   dissect;
	sensor_opt_persist   persist;
	sensor_opt_survey    survey;
	sensor_opt_balancing balancing;
} sensor_options_t;


/*--------------------------------------*/
typedef struct sensor *sensor_t;


sensor_captured_t *init_captured(uint8_t *buffer, int len);
sensor_dissected_t *init_dissected(int content_length, int payload_length);
void destroy_captured(sensor_captured_t *captured);
void destroy_dissected(sensor_dissected_t *dissected);

sensor_t sensor_init();
void sensor_destroy(sensor_t config);
int sensor_set_options(sensor_t config, sensor_options_t options);
int sensor_set_dissection_default(sensor_t config);
int sensor_set_persist_callback(sensor_t config, sensor_persist_f callback);
int sensor_loop(sensor_t config);
void sensor_breakloop(sensor_t config);


#endif /*SENSOR_H*/
