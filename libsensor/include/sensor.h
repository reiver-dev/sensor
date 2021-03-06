#ifndef SENSOR_H
#define SENSOR_H
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <net/if.h>
#include <sys/time.h>

#ifdef __cplusplus
#define CPP_EXTERN_C_BEGIN extern "C" {
#define CPP_EXTERN_C_END }
#else
#define CPP_EXTERN_C_BEGIN
#define CPP_EXTERN_C_END
#endif

CPP_EXTERN_C_BEGIN

struct sensor_header_s {
	struct timeval ts;
	uint32_t caplen;
	uint32_t len;
};

typedef struct sensor_captured_s {
	struct sensor_header_s header;
	uint8_t* buffer;
} sensor_captured_t;

typedef int (*sensor_persist_f)(sensor_captured_t *captured);


/* Options */
typedef struct {
	char device_name[IF_NAMESIZE];
	bool promiscuous;
	uint32_t buffersize;
	uint32_t timeout;
} sensor_opt_capture;

typedef struct {
	char device_name[IF_NAMESIZE];
	uint32_t timeout;
} sensor_opt_persist;

typedef struct {
	uint32_t survey_timeout;
	uint32_t survey_initial_timeout;
	uint32_t disconnect_timeout;
	bool enable_redirect;
	bool enable_modify_routing;
	uint32_t modify_routing_timeout;
} sensor_opt_survey;

typedef struct {
	char device_name[IF_NAMESIZE];
	uint32_t timeout;
	uint32_t initial_timeout;
	uint32_t info_timeout;
	uint32_t session_timeout;
	uint32_t load_count;
	uint32_t load_interval;
} sensor_opt_balancing;

typedef struct {
	sensor_opt_capture   capture;
	sensor_opt_persist   persist;
	sensor_opt_survey    nodes;
	sensor_opt_balancing balancing;
} sensor_options_t;


/*--------------------------------------*/
typedef struct sensor *sensor_t;

sensor_t sensor_init();
void sensor_destroy(sensor_t config);
int sensor_set_options(sensor_t config, sensor_options_t options);
int sensor_set_persist_callback(sensor_t config, sensor_persist_f callback);
int sensor_main(sensor_t config);
void sensor_breakloop(sensor_t config);

CPP_EXTERN_C_END

#endif /*SENSOR_H*/
