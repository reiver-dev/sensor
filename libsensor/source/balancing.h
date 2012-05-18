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
void balancing_disconnect(Balancer self);
void balancing_receive_service(Balancer self);

uint8_t balancing_get_state(Balancer self);
time_t balancing_get_created(Balancer self);
ArrayList balancing_get_owned(Balancer self);

void balancing_release_node(Balancer self, uint32_t ip4c);
void balancing_take_node_from(Balancer self, uint32_t ip4s, uint32_t ip4c);
void balancing_sensor_info_refreshed(Balancer self, uint32_t ip4s);
void balancing_node_owned(Balancer self, uint32_t ip4s, uint32_t ip4c, uint32_t load);

void balancing_add_load(Balancer self, uint8_t *buffer, size_t length);
void balancing_count_load(Balancer self);
bool balancing_is_load_ready(Balancer self, uint32_t ip4addr);

bool balancing_is_valid_addreses(Balancer self, uint32_t ip4from, uint32_t ip4to);
bool balancing_is_in_session(Balancer self, uint32_t ip);

void balancing_init_sensor_session(Balancer self, uint32_t ip4, time_t created);
void balancing_break_sensor_session(Balancer self, uint32_t ip4);

#endif /* BALANCING_H_ */
