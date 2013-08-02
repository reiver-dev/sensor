#ifndef REDIRECT_H_
#define REDIRECT_H_

#include <netinet/ether.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Ip4NetInfo {
	struct in_addr local;
	struct in_addr netmask;
	struct in_addr gateway;
};

struct Ip6NetInfo {
	struct in6_addr local;
	struct in6_addr netmask;
	struct in6_addr gateway;
};

struct InterfaceInfo {
	struct ether_addr hw;
	struct Ip4NetInfo ip4;
	struct Ip6NetInfo ip6;
};


struct InterfaceInfo read_interface_info(const char* interfaceName);

uint8_t* get_current_mac(int sock, const char* interfaceName);
uint8_t* get_current_mac_r(int sock, const char* interfaceName, uint8_t* hwaddr);
struct in_addr get_current_address(int sock, const char* interfaceName);
struct in_addr get_current_netmask(int sock, const char* interfaceName);
uint8_t* read_arp_ip_to_mac(int sock, const char* interfaceName, uint32_t ipaddress);
uint8_t* read_arp_ip_to_mac_r(int sock, const char* interfaceName, uint32_t ipaddress, uint8_t hwaddr[ETH_ALEN]);
uint32_t read_arp_mac_to_ip(int sock, const char* interfaceName, uint8_t hwaddress[ETH_ALEN]);
uint32_t read_default_gateway(const char* interfaceName);

#ifdef __cplusplus
}
#endif

#endif /* REDIRECT_H_ */
