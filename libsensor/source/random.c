#include "random.h"

#include <unistd.h>
#include <stdlib.h>

#include "clock.h"

void random_seed() {
	pid_t pid = getpid();
	srand(pid * get_now_usec());
}

uint16_t random_get16() {
	return rand();
}

uint32_t random_get32() {
	return rand();
}

uint64_t random_get64() {
	uint32_t arr[2] = {rand(), rand()};
	uint64_t res = *(uint64_t *)arr;
	return res;
}

void random_getbuf(uint8_t *buf, size_t len) {
	for (size_t i; i < len; i++) {
		buf[i] = rand();
	}
}
