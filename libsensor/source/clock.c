
#include "clock.h"
#include <time.h>
#include <assert.h>


static uint64_t timespec_to_usec(struct timespec time) {
	return time.tv_sec * (uint64_t)1E6 + time.tv_nsec / (uint64_t) 1E3;
}


uint64_t get_now_usec() {
	struct timespec ts;
	int rc = clock_gettime(CLOCK_MONOTONIC, &ts);
	assert(rc);
	return timespec_to_usec(ts);
}



