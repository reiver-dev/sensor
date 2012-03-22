#ifndef BALANCING_H_
#define BALANCING_H_

#include "sensor.h"

//void balancing_init(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[6]);
typedef struct balancer *Balancer;

Balancer balancing_init(sensor_t *config);
void balancing_survey(Balancer self, int packet_sock);
void balancing_destroy(Balancer self);
void balancing_check_response(Balancer self, uint8_t *buffer, int length);
void balancing_process(Balancer self);

#endif /* BALANCING_H_ */
