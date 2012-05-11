#ifndef BALANCING_H_
#define BALANCING_H_

#include "sensor_private.h"
#include "arraylist.h"

enum BALANCING_STATE {
	STATE_BEGIN,
	STATE_WAIT_SENSORS,
	STATE_ALONE,
	STATE_COUPLE,
};

//void balancing_init(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[6]);
typedef struct balancer *Balancer;

Balancer balancing_init(sensor_t config);
void balancing_destroy(Balancer self);
bool balancing_filter_response(Balancer self, uint8_t *buffer, int length);
void balancing_process(Balancer self);
void balancing_receive_service(Balancer self);

uint8_t balancing_get_state(Balancer self);
ArrayList balancing_get_owned(Balancer self);

void balancing_take_node(Balancer self, uint32_t ip4addr);
void balancing_node_owned(Balancer self, uint32_t ip4s, uint32_t ip4c);

void balancing_add_load(Balancer self, uint8_t *buffer, int length);
void balancing_count_load(Balancer self, uint32_t load_interval, uint32_t load_count);

#endif /* BALANCING_H_ */
