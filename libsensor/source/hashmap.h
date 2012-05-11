#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct HashMap *HashMap;
struct HashMapPair {
	void *key;
	void *value;
};

typedef uint32_t (* HashMapHash)(const void *);
typedef bool (* HashMapEquals)(const void *, const void *);
typedef void (* HashMapDestroyer)(void *);

uint32_t HashMap_times33(const uint8_t *data, size_t size);
uint32_t HashMap_sdbm(const uint8_t *data, size_t size);

HashMap HashMap_init(HashMapHash hash, HashMapEquals equals, HashMapDestroyer key, HashMapDestroyer value);
HashMap HashMap_initString(HashMapDestroyer key, HashMapDestroyer value);
HashMap HashMap_initInt32(HashMapDestroyer key, HashMapDestroyer value);

void HashMap_destroy(HashMap self);

size_t HashMap_size(HashMap self);
void **HashMap_getKeys(HashMap self);
void **HashMap_getValues(HashMap self);
struct HashMapPair *HashMap_getPairs(HashMap self);

void *HashMap_get(HashMap self, void *key);
bool HashMap_contains(HashMap self, void *key);
bool HashMap_add(HashMap self, void *key, void *val);
void HashMap_addInt32(HashMap self, uint32_t key, void *val);
void HashMap_remove(HashMap self, void *key);

#endif /* HASHMAP_H */
