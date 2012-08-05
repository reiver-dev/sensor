#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>

#include <netinet/ether.h>
#include <netinet/in.h>

#include "netinfo.h"
#include "debug.h"
#include "util.h"


struct InterfaceAddress read_interface_address(const char* interfaceName) {
	struct InterfaceAddress address;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	get_current_mac_r(sock, interfaceName, address.hwaddr);
	address.ip4addr = get_current_address(sock, interfaceName);
	address.netmask = get_current_netmask(sock, interfaceName);
	address.gateway = read_default_gateway(interfaceName);
	close(sock);

	return address;
}

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


