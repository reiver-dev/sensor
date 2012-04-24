#include <stdlib.h>
#include "packet_extract.h"

struct ether_header *packet_map_ether(uint8_t *buffer, int len) {
	if (len > sizeof(struct ether_header))
		return (struct ether_header*) (buffer);
	return NULL;
}

struct iphdr *packet_map_ip(uint8_t *buffer, int len) {
	struct ether_header *ether = packet_map_ether(buffer, len);

	if (ether && ETH_P_IP && len >= sizeof(struct iphdr *)) {
		return (struct iphdr *)(ether + sizeof(struct ether_header));
	}

	return NULL;
}

struct udphdr *packet_map_udp(uint8_t *buffer, int len) {
	struct iphdr *ip = packet_map_ip(buffer, len);
	if (ip) {
		int iplen = ip->ihl * 4;
		void *next = ip + iplen;
		int left = len - ((uintptr_t)next - (uintptr_t)buffer);

		if (left >= sizeof(struct udphdr) + ip->ihl * 4 && ip->protocol == IPPROTO_UDP) {
			return (struct udphdr *) (ip + ip->ihl * 4);
		}
	}

	return NULL;
}

struct tcphdr *packet_map_tcp(uint8_t *buffer, int len) {
	struct iphdr *ip = packet_map_ip(buffer, len);
	if (ip) {
		int iplen = ip->ihl * 4;
		void *next = ip + iplen;
		int left = len - ((uintptr_t)next - (uintptr_t)buffer);

		if (left >= sizeof(struct tcphdr) && ip->protocol == IPPROTO_TCP) {
			return (struct tcphdr *) (ip + iplen);
		}
	}
	return NULL;
}


uint8_t *packet_map_tcp_payload(uint8_t *buffer, int len) {
	struct tcphdr *tcp = packet_map_tcp(buffer, len);

	if (tcp) {
		int tcplen = tcp->doff * 4;
		void *next = tcp + tcplen;
		int left = len - ((uintptr_t)next - (uintptr_t)buffer);

		if (left > 0)
			return next;
	}

	return NULL;
}

uint8_t *packet_map_udp_payload(uint8_t *buffer, int len) {
	struct udphdr *udp = packet_map_udp(buffer, len);

	if (udp) {
		int updlen = sizeof(*udp);
		void *next = udp + updlen;
		int left = len - ((uintptr_t)next - (uintptr_t)buffer);

		if (left > 0)
			return next;
	}

	return NULL;

}


uint8_t *packet_map_payload(uint8_t *buffer, int len) {
	uint8_t *payload;

	payload = packet_map_udp_payload(buffer, len);
	if (payload) {
		return payload;
	}

	payload = packet_map_tcp_payload(buffer, len);
	if (payload) {
		return payload;
	}

	return NULL;
}


