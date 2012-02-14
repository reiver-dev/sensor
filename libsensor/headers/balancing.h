#ifndef BALANCING_H_
#define BALANCING_H_

void balancing_initNodes(const uint32_t ip4addr, const uint32_t netmask, const uint8_t hwaddr[6]);
void balancing_survey(int packet_sock);
void balancing_destroyNodes();

#endif /* BALANCING_H_ */
