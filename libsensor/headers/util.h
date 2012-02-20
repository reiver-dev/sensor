#ifndef UTIL_H_
#define UTIL_H_
#include <arpa/inet.h>
#include <netinet/ether.h>

char *Ip4ToStr(const uint32_t ip);

char *EtherToStr(const uint8_t eth[ETH_ALEN]);

int bitcount(unsigned int n);

#endif /* UTIL_H_ */
