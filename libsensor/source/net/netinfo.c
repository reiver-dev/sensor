#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <errno.h>

#include <ifaddrs.h>
#include <netdb.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>

#include <netinet/ether.h>
#include <netinet/in.h>

#include "netinfo.h"
#include "base/debug.h"
#include "base/util.h"


bool read_interface_info(const char* interfaceName, struct InterfaceInfo *info) {

	get_current_hwaddr(interfaceName, info->hw.ether_addr_octet);

	get_current_address(AF_INET, interfaceName, &info->ip4.local);
	get_current_netmask(AF_INET, interfaceName, &info->ip4.netmask);
	info->ip4.gateway.s_addr = read_default_gateway(interfaceName);

	get_current_address(AF_INET6, interfaceName, &info->ip6.local);
	get_current_netmask(AF_INET6, interfaceName, &info->ip6.netmask);

	return info;
}

bool get_current_hwaddr(const char* interfaceName, uint8_t* hwaddr) {
	struct ifreq interface;
	int sock;
	int result;

	strcpy(interface.ifr_name, interfaceName);

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if ((result = ioctl(sock, SIOCGIFHWADDR, &interface)) == -1) {
		char errbuf[512];
		DERROR("%s\n", strerror_r(errno, errbuf, sizeof(errbuf)));
	} else {
		memcpy(hwaddr, (uint8_t*)interface.ifr_hwaddr.sa_data, ETH_ALEN);
	}

	close(sock);

	return result != -1;
}


bool get_interface_inet_address_str(int family, const char* interfaceName, char *address, size_t len) {
	struct ifaddrs *ifas, *ifa;
	int rc = 0;
	bool result = false;

	assert(family == AF_INET || family == AF_INET6);

	if (getifaddrs(&ifas) == -1) {
		char errbuf[512];
		DERROR("%s\n", strerror_r(errno, errbuf, sizeof(errbuf)));
		return false;
	}

	for (ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		if (strncmp(ifa->ifa_name, interfaceName, IF_NAMESIZE) && ifa->ifa_addr->sa_family == family) {
			size_t addrsz = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
			rc = getnameinfo(ifa->ifa_addr, addrsz, address, len, NULL, 0, NI_NUMERICHOST);
			if (!rc) {
				result = true;
			} else {
				DERROR("Can't read address of (%s) interface: %s\n", gai_strerror(rc));
			}
			break;
		}
	}

	freeifaddrs(ifas);

	if (!ifa && !result)
		DWARNING("Cant find (%s) interface with (%s) family\n", interfaceName, family == AF_INET ? "INET" : "INET6");

	return result;

}

bool get_current_address(int family, const char* interfaceName, void *address) {
	struct ifreq interface;
	int sock;
	int result;

	assert(family == AF_INET || family == AF_INET6);

	strcpy(interface.ifr_name, interfaceName);
	interface.ifr_addr.sa_family = family;

	sock = socket(family, SOCK_DGRAM, 0);

	if ((result = ioctl(sock, SIOCGIFADDR, &interface)) == -1) {
		char errbuf[512];
		DERROR("%s\n", strerror_r(errno, errbuf, sizeof(errbuf)));
	} else {
		if (family == AF_INET) {
			*((struct in_addr *)address) = ((struct sockaddr_in *)&interface.ifr_addr)->sin_addr;
		} else {
			*((struct in6_addr *)address) = ((struct sockaddr_in6 *)&interface.ifr_addr)->sin6_addr;
		}
	}

	close(sock);

	return result != -1;
}

bool get_current_netmask(int family, const char* interfaceName, void *netmask) {
	struct ifreq interface;
	int sock;
	int result;

	assert(family == AF_INET || family == AF_INET6);

	strcpy(interface.ifr_name, interfaceName);
	interface.ifr_addr.sa_family = family;

	sock = socket(family, SOCK_DGRAM, 0);

	if ((result = ioctl(sock, SIOCGIFNETMASK, &interface)) == -1) {
		char errbuf[512];
		DERROR("%s\n", strerror_r(errno, errbuf, sizeof(errbuf)));
	} else {
		if (family == AF_INET) {
			*((struct in_addr *)netmask) = ((struct sockaddr_in *)&interface.ifr_addr)->sin_addr;
		} else {
			*((struct in6_addr *)netmask) = ((struct sockaddr_in6 *)&interface.ifr_addr)->sin6_addr;
		}
	}

	close(sock);

	return result != -1;

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
	char buffer[512];
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


