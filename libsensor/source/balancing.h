#ifndef BALANCING_H_
#define BALANCING_H_

#include "sensor_private.h"

//void balancing_init(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[6]);
typedef struct balancer *Balancer;

Balancer balancing_init(sensor_t config);
void balancing_destroy(Balancer self);
bool balancing_process_response(Balancer self, uint8_t *buffer, int length);
void balancing_process(Balancer self);

bool balancing_count_load(Balancer self, uint8_t *buffer, int length, uint32_t load_interval, uint32_t load_count);

#endif /* BALANCING_H_ */
