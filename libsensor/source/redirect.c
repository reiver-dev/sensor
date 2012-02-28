#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>

#include <netinet/ether.h>
#include <netinet/in.h>

/* to get default route */
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <libnetlink.h>
#include <net/route.h>

#include <proc/readproc.h>

#include "redirect.h"
#include "debug.h"


uint8_t* get_current_mac(int sock, const char* interfaceName) {
	static uint8_t hwaddr[ETH_ALEN] = {0};
	return get_current_mac_r(sock, interfaceName, hwaddr);
}

uint8_t* get_current_mac_r(int sock, const char* interfaceName, uint8_t* hwaddr) {
	struct ifreq interface;
	strcpy(interface.ifr_name, interfaceName);

	if (ioctl(sock, SIOCGIFHWADDR, &interface) == -1) {
		return 0;
	}

	memcpy(hwaddr, (uint8_t*)interface.ifr_hwaddr.sa_data, ETH_ALEN);
	return hwaddr;
}


uint32_t get_current_address(int sock, const char* interfaceName) {

	struct ifreq interface;
	strcpy(interface.ifr_name, interfaceName);
	interface.ifr_addr.sa_family = AF_INET;

	if (ioctl(sock, SIOCGIFADDR, &interface) == -1) {
		return 0;
	}

	uint32_t address = ((struct sockaddr_in *)&interface.ifr_addr)->sin_addr.s_addr;
	return address;
}

/*
void get_current_address6(int sock, const char* interfaceName) {

	struct ifreq interface;
	strcpy(interface.ifr_name, interfaceName);
	interface.ifr_addr.sa_family = AF_INET6;

	if (ioctl(sock, SIOCGIFADDR, &interface) == -1) {
		return;
	}

	char temp_buf[32];
	struct in6_addr address = ((struct sockaddr_in6 *)&interface.ifr_addr)->sin6_addr;


}
*/


uint32_t get_current_netmask(int sock, const char* interfaceName) {
	struct ifreq interface;
	strcpy(interface.ifr_name, interfaceName);
	interface.ifr_addr.sa_family = AF_INET;

	if (ioctl(sock, SIOCGIFNETMASK, &interface) == -1) {
		return 0;
	}

	uint32_t netmask = ((struct sockaddr_in *)&interface.ifr_addr)->sin_addr.s_addr;
	return netmask;

}

uint8_t* read_arp_ip_to_mac(int sock, const char* interfaceName, uint32_t ipaddress) {
	static uint8_t hwaddr[ETH_ALEN] = {0};
	return read_arp_ip_to_mac_r(sock, interfaceName, ipaddress, hwaddr);
}

uint8_t* read_arp_ip_to_mac_r(int sock, const char* interfaceName, uint32_t ipaddress, uint8_t hwaddr[ETH_ALEN]) {

	struct arpreq arp;
	strcpy(arp.arp_dev, interfaceName);

	struct sockaddr_in *address = (struct sockaddr_in *)&arp.arp_pa;
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = ipaddress;

	if (ioctl(sock, SIOCGARP, &arp) == -1) {
		return 0;
	}

	memcpy(hwaddr, (uint8_t*)arp.arp_ha.sa_data, ETH_ALEN);
	return hwaddr;

}


uint32_t read_arp_mac_to_ip(int sock, const char* interfaceName, uint8_t hwaddress[ETH_ALEN]) {
	struct arpreq arp;
	strcpy(arp.arp_dev, interfaceName);
	memcpy(arp.arp_ha.sa_data, hwaddress, ETH_ALEN);

	if (ioctl(sock, SIOCGARP, &arp) == -1) {
		return 0;
	}

	uint32_t address = ((struct sockaddr_in *)&arp.arp_pa)->sin_addr.s_addr;
	return address;
}

/*
uint32_t read_default_gateway(const char* interfaceName) {

	int sock;
	if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
		return(-1);
	}

	struct {
		struct nlmsghdr header;
		struct ifinfomsg msg;
		char data[512];
	} request;

	memset(&request, '\0', sizeof(request));

	request.header.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	request.header.nlmsg_len   = NLMSG_LENGTH(sizeof(request));
	request.header.nlmsg_pid   = getpid();
	request.header.nlmsg_seq   = 1;
	request.header.nlmsg_type  = RTM_GETROUTE;

	request.msg.ifi_family     = AF_INET;

	if (send(sock, &request, request.header.nlmsg_len, 0) < 0) {
		return -1;
	}
z
	return 0;
}
*/


static uint8_t hex_to_uint8(const char hex) {
	uint8_t result;

	switch (hex) {
	case '0': result =  0; break;
	case '1': result =  1; break;
	case '2': result =  2; break;
	case '3': result =  3; break;
	case '4': result =  4; break;
	case '5': result =  5; break;
	case '6': result =  6; break;
	case '7': result =  7; break;
	case '8': result =  8; break;
	case '9': result =  9; break;
	case 'A': result = 10; break;
	case 'B': result = 11; break;
	case 'C': result = 12; break;
	case 'D': result = 13; break;
	case 'E': result = 14; break;
	case 'F': result = 15; break;
	}

	return result;
}

static uint32_t hex_to_uint32(const char *hex) {
	uint8_t a[] = {
		(hex_to_uint8(hex[0]) << 4) | hex_to_uint8(hex[1]),
		(hex_to_uint8(hex[2]) << 4) | hex_to_uint8(hex[3]),
		(hex_to_uint8(hex[4]) << 4) | hex_to_uint8(hex[5]),
		(hex_to_uint8(hex[6]) << 4) | hex_to_uint8(hex[7])
	};
	return *(uint32_t *)a;
}

uint32_t read_default_gateway(const char* interfaceName) {

	FILE *route = fopen("/proc/net/route", "r");
	if (route == NULL) {
		DERROR("Can't open file /proc/net/route; errno: %s", errno);
		return 0;
	}

	uint32_t result = 0;
	char buffer[256];
	fgets(buffer, sizeof(buffer), route); /* read header */
	while (fgets(buffer, sizeof(buffer), route)) {
		char *iface   = strtok(buffer, "\t");
		char *destHEX = strtok(0, "\t");
		char *gwHEX   = strtok(0, "\t");
		if (!strcmp(iface, interfaceName) && !strcmp(destHEX, "00000000")) {
			result = htonl(hex_to_uint32(gwHEX));
			break;
		}
	}

	fclose(route);

	return result;

}


