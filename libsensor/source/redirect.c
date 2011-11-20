#include <string.h>
#include <stdint.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>

#include <netinet/ether.h>
#include <netinet/in.h>
//#include <arpa/inet.h>

#include "redirect.h"

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

