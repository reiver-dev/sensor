#ifndef REDIRECT_H_
#define REDIRECT_H_

uint8_t* get_current_mac(int sock, const char* interfaceName);
uint8_t* get_current_mac_r(int sock, const char* interfaceName, uint8_t* hwaddr);
uint32_t get_current_address(int sock, const char* interfaceName);
uint32_t get_current_netmask(int sock, const char* interfaceName);
uint8_t* read_arp_ip_to_mac(int sock, const char* interfaceName, uint32_t ipaddress);
uint8_t* read_arp_ip_to_mac_r(int sock, const char* interfaceName, uint32_t ipaddress, uint8_t hwaddr[ETH_ALEN]);
uint32_t read_arp_mac_to_ip(int sock, const char* interfaceName, uint8_t hwaddress[ETH_ALEN]);
uint32_t read_default_gateway(const char* interfaceName);
#endif /* REDIRECT_H_ */
