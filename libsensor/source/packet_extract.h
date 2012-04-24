#ifndef PACKET_EXTRACT_H_
#define PACKET_EXTRACT_H_

#include <stdint.h>

#include <netinet/ip.h>
#include <netinet/ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

struct ether_header *packet_map_ether(uint8_t *buffer, int len);
struct iphdr *packet_map_ip(uint8_t *buffer, int len);
struct udphdr *packet_map_udp(uint8_t *buffer, int len);
struct tcphdr *packet_map_tcp(uint8_t *buffer, int len);

uint8_t *packet_map_tcp_payload(uint8_t *buffer, int len);
uint8_t *packet_map_udp_payload(uint8_t *buffer, int len);
uint8_t *packet_map_payload(uint8_t *buffer, int len);




#endif /* PACKET_EXTRACT_H_ */
