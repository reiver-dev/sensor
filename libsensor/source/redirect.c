#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <net/if.h>
#include <sys/ioctl.h>

#include <netinet/ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>

uint8_t* get_current_mac(int sock, const char* interfaceName) {

	struct ifreq interface;
	strcpy(interface.ifr_name, interfaceName);

	if (ioctl(sock, SIOCGIFHWADDR, &interface) == -1) {
		return 0;
	}

	uint8_t* hwaddr = (uint8_t*)interface.ifr_hwaddr.sa_data;
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
