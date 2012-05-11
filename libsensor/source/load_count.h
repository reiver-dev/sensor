#ifndef LOAD_COUNT_H_
#define LOAD_COUNT_H_

#include <stdint.h>
#include "arraylist.h"
#include "balancing.h"
#include "hashmap.h"

void load_bytes_add(ArrayList momentLoads, int len);
void load_count(HashMap clientMomentLoads, uint32_t load_interval, uint32_t load_count);

#endif /* LOAD_COUNT_H_ */
