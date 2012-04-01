#ifndef UTIL_H_
#define UTIL_H_
#include <stdint.h>

int RemoveElement(void *array, size_t size, int length, int index);
void *Reallocate(void *array, size_t size, int length, int index, int period);

char *Ip4ToStr(const uint32_t ip);

char *EtherToStr(const uint8_t eth[ETH_ALEN]);

int bitcount(unsigned int n);

int bind_socket_to_interface(int sock, char *interfaceName);

void AddToBuffer(uint8_t **bufpointer, void *data, size_t ofData);

void AddToBuffer32(uint8_t **bufpointer, uint32_t data);
void AddToBuffer16(uint8_t **bufpointer, uint16_t data);
void AddToBuffer8(uint8_t **bufpointer, uint8_t data);
void AddToBuffer4(uint8_t **bufpointer, uint8_t data8, uint8_t data4);
void AddToBuffer1(uint8_t *bufpointer, uint8_t data, int place);

uint32_t GetFromBuffer32(uint8_t **bufpointer);
uint16_t GetFromBuffer16(uint8_t **bufpointer);
uint8_t GetFromBuffer8(uint8_t **bufpointer);
void GetFromBuffer4(uint8_t **bufpointer, uint8_t *data8, uint8_t *data4);
uint8_t GetFromBuffer1(const uint8_t *bufpointer, int place);

#define ADD_TO_BUFFER(buf, data, size) (memcpy(buf, data, size), buf+=size)

#endif /* UTIL_H_ */
