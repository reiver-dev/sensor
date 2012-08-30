#include <stdlib.h>
#include <stdbool.h>
#include "packet_extract.hpp"

static void *next(void *header, size_t header_len) {
	if (!header) {
		return NULL;
	}
	return (uint8_t *)header + header_len;
}

static bool has_space(void *buffer, size_t buffer_len, void *header, size_t header_len) {
	void *offset = (uint8_t *)header + header_len;
	size_t distance = (uintptr_t)offset - (uintptr_t)buffer;
	return buffer_len - distance >= 0;
}

struct ether_header *packet_map_ether(uint8_t *buffer, size_t len) {
	if (len >= sizeof(struct ether_header))
		return (struct ether_header*) (buffer);
	return NULL;
}

struct iphdr *packet_map_ip(uint8_t *buffer, size_t len) {
	struct ether_header *ethernet = packet_map_ether(buffer, len);
	if (!ethernet) {
		return NULL;
	}

	struct iphdr *ipheader = (struct iphdr *)next(ethernet, sizeof(*ethernet));
	if (has_space(buffer, len, ipheader, ipheader->ihl * 4)) {
		return ipheader;
	}

	return NULL;
}

struct udphdr *packet_map_udp(uint8_t *buffer, size_t len) {
	struct iphdr *ip = packet_map_ip(buffer, len);
	if (!(ip && ip->protocol == IPPROTO_UDP)) {
		return NULL;
	}

	struct udphdr *udpheader = (struct udphdr *)next(ip, ip->ihl * 4);
	if (has_space(buffer, len, udpheader, sizeof(*udpheader))) {
		return udpheader;
	}

	return NULL;
}

struct tcphdr *packet_map_tcp(uint8_t *buffer, size_t len) {
	struct iphdr *ip = packet_map_ip(buffer, len);
	if (!(ip && ip->protocol == IPPROTO_TCP)) {
		return NULL;
	}

	struct tcphdr *tcpheader = (struct tcphdr *)next(ip, ip->ihl * 4);
	if (has_space(buffer, len, tcpheader, tcpheader->doff * 4)) {
		return tcpheader;
	}

	return NULL;
}

uint8_t *packet_map_payload(uint8_t *buffer, size_t len) {
	uint8_t *payload = NULL;

	struct udphdr *udpheader = packet_map_udp(buffer, len);
	if (udpheader) {
		payload = (uint8_t *)next(udpheader, sizeof(*udpheader));
	}

	struct tcphdr *tcpheader = packet_map_tcp(buffer, len);
	if (tcpheader) {
		payload = (uint8_t *)next(tcpheader, tcpheader->doff * 4);
	}

	return payload;
}


