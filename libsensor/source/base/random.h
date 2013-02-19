#ifndef RANDOM_H_
#define RANDOM_H_

#include <stdint.h>
#include <stddef.h>

void random_seed();
uint32_t random_get32();
uint64_t random_get64();
void random_getbuf(uint8_t *buf, size_t len);

#endif /* RANDOM_H_ */
