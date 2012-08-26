#ifndef TSHASHMAP_H_
#define TSHASHMAP_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>



typedef struct TsHashMap *TsHashMap;

TsHashMap TsHashMap_init(size_t key, size_t val);

void TsHashMap_destroy(TsHashMap self);

size_t TsHashMap_size(TsHashMap self);

bool TsHashMap_add(TsHashMap self, void *key, void *val);
bool TsHashMap_replace(TsHashMap self, void *key, void *val);
bool TsHashMap_get(TsHashMap self, void *key, void *val);
void *TsHashMap_getAll(TsHashMap self, size_t *count);
bool TsHashMap_steal(TsHashMap self, void *key, void *val);
bool TsHashMap_contains(TsHashMap self, void *key);
bool TsHashMap_remove(TsHashMap self, void *key);

#endif /* TSHASHMAP_H_ */
