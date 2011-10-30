#ifndef SENSOR_H
#define SENSOR_H
#include <stdint.h>
#include "queue.h"


typedef int (*sensor_persist_f)(Queue_t in);


typedef struct sensor sensor_t;

sensor_t sensor_init();
int sensor_set_options(sensor_t config, char *device, bool is_promisc, uint8_t capture_timeout);
int sensor_set_dissection_simple(sensor_t config);
int sensor_loop(sensor_t config, sensor_persist_f callback);
int sensor_breakloop(sensor_t config);
int sensor_destroy(sensor_t config);



int create_socket();
int close_socket(int socket);
int set_iface_promiscuous(int sock, const char* interfaceName, bool state);
int get_next_packet(int sock, int seconds);
uint8_t dissect(uint8_t* packet, int length);

#endif /*SENSOR_H*/
