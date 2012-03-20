#ifndef UTIL_H_
#define UTIL_H_
#include <stdint.h>

char *Ip4ToStr(const uint32_t ip);

char *EtherToStr(const uint8_t eth[ETH_ALEN]);

int bitcount(unsigned int n);

int bind_socket_to_interface(int sock, char *interfaceName);

void AddToBuffer(uint8_t *bufpointer, void *data, size_t ofData);
#define ADD_TO_BUFFER(buf, x) AddToBuffer(buf, x, sizeof(typeof(x)))

#endif /* UTIL_H_ */
