#ifndef DISSECT_H_
#define DISSECT_H_


#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>


#include "queue.h"

int sensor_dissect_simple(Queue_t *in, Queue_t *out);
//int sensor_dissect_xml(Queue_t *in, Queue_t *out);

char* dissect_ethernet(struct ether_header *header);
char* dissect_tcp(struct tcphdr *header);
char* dissect_udp(struct udphdr *header);
char* dissect_ip(struct iphdr *header);
char* dissect_ethernet(struct ether_header *header);
char* dissect_arp(struct ether_arp *header);
char* dissect_icmp(struct icmphdr *header);

#endif /* DISSECT_H_ */
