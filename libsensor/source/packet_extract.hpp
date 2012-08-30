#ifndef PACKET_EXTRACT_H_
#define PACKET_EXTRACT_H_

#include <cstdint>

#include <netinet/ip.h>
#include <netinet/ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

struct ether_header *packet_map_ether(uint8_t *buffer, size_t len);
struct iphdr *packet_map_ip(uint8_t *buffer, size_t len);
struct udphdr *packet_map_udp(uint8_t *buffer, size_t len);
struct tcphdr *packet_map_tcp(uint8_t *buffer, size_t len);
uint8_t *packet_map_payload(uint8_t *buffer, size_t len);




#endif /* PACKET_EXTRACT_H_ */
