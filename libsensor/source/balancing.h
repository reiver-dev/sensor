#ifndef BALANCING_H_
#define BALANCING_H_

//void balancing_init(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[6]);
void balancing_init(sensor_t *config);
void balancing_survey(int packet_sock);
void balancing_destroy();
void balancing_check_response(const uint8_t *buffer, int length);

#endif /* BALANCING_H_ */
