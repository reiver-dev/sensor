#ifndef REDIRECT_H_
#define REDIRECT_H_

#include <netinet/ether.h>
#include <netinet/in.h>

struct InterfaceAddress {
	uint32_t ip4addr;
	uint32_t netmask;
	uint8_t hwaddr[ETH_ALEN];
	uint32_t gateway;
};

struct IntenetAddress4 {
	struct in_addr local;
	struct in_addr netmask;
	struct in_addr gateway;
	struct ether_addr hw;
};

struct IntenetAddress6 {
	struct in6_addr local;
	struct in6_addr netmask;
	struct in6_addr gateway;
	struct ether_addr hw;
};

struct InterfaceAddress read_interface_address(const char* interfaceName);

uint8_t* get_current_mac(int sock, const char* interfaceName);
uint8_t* get_current_mac_r(int sock, const char* interfaceName, uint8_t* hwaddr);
uint32_t get_current_address(int sock, const char* interfaceName);
uint32_t get_current_netmask(int sock, const char* interfaceName);
uint8_t* read_arp_ip_to_mac(int sock, const char* interfaceName, uint32_t ipaddress);
uint8_t* read_arp_ip_to_mac_r(int sock, const char* interfaceName, uint32_t ipaddress, uint8_t hwaddr[ETH_ALEN]);
uint32_t read_arp_mac_to_ip(int sock, const char* interfaceName, uint8_t hwaddress[ETH_ALEN]);
uint32_t read_default_gateway(const char* interfaceName);
#endif /* REDIRECT_H_ */
