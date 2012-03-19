#ifndef UTIL_H_
#define UTIL_H_

char *Ip4ToStr(const uint32_t ip);

char *EtherToStr(const uint8_t eth[ETH_ALEN]);

int bitcount(unsigned int n);

int bind_socket_to_interface(int sock, char *interfaceName);

#endif /* UTIL_H_ */
