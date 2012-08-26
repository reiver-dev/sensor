#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>


#include "tshashmap.h"

#define EXPAND_FACTOR 2
#define CAPACITY_PEAK 0.75
#define SEGMENT_LENGTH 16

#define NOT_NULL(x) x != 0 ? x : 1
#define GET_SEGMENTS(x) NOT_NULL(x / SEGMENTLENGTH)

struct Bucket {
	struct Bucket *next;
	uint32_t hash;
	void *key;
	void *val;
};

struct TsHashMap {
	struct Bucket **data;

	pthread_rwlock_t lock;

	size_t length;
	size_t capacity;

	size_t key_size;
	size_t val_size;
};


#define MAP_LOCK_INIT(map)    pthread_rwlock_init(&map->lock, NULL)
#define MAP_LOCK_READ(map)    pthread_rwlock_rdlock(&map->lock)
#define MAP_LOCK_WRITE(map)   pthread_rwlock_wrlock(&map->lock)
#define MAP_LOCK_UNLOCK(map)  pthread_rwlock_unlock(&map->lock)
#define MAP_LOCK_DESTROY(map) pthread_rwlock_destroy(&map->lock)

static struct Bucket *bucket_init(size_t key, size_t val) {
	char *buf = malloc(sizeof(struct Bucket) + key + val);
	struct Bucket *bucket = (struct Bucket *)buf;
	bucket->key  = buf + sizeof(struct Bucket);
	bucket->val  = buf + sizeof(struct Bucket) + key;
	bucket->next = NULL;
	bucket->hash = 0;
	return bucket;
}

static inline bool bucket_equals(TsHashMap self, struct Bucket *bucket, uint32_t hash, void *key) {
	bool f1 = bucket->hash == hash;
	bool f2 = memcpy(bucket->key, key, self->key_size) == 0;
	if (f1 && f2)
		return true;
	return false;
}

static inline size_t get_place(size_t len, uint32_t hash) {
	return hash & (len - 1);
}

static struct Bucket *get_bucket(TsHashMap self, uint32_t hash, void *key) {
	size_t index = get_place(self->length, hash);
	struct Bucket *bucket;
	for (bucket = self->data[index]; bucket; bucket = bucket->next) {
		if (bucket_equals(self, bucket, hash, key))
			break;
	}
	return bucket;
}

static struct Bucket *steal_bucket(TsHashMap self, uint32_t hash, void *key) {
	size_t index = get_place(self->length, hash);

	struct Bucket *bucket;
	struct Bucket *prev = NULL;

	for (bucket = self->data[index]; bucket; bucket = bucket->next) {
		if (bucket_equals(self, bucket, hash, key)) {
			if (prev) {
				prev->next = bucket->next;
			} else {
				self->data[index] = bucket->next;
			}
			break;
		}
		prev = bucket;
	}
	return bucket;
}


static void expand_map(TsHashMap self) {
	size_t newLen = EXPAND_FACTOR * self->length;

	struct Bucket **newData = malloc(newLen * sizeof(uintptr_t));
	memset(newData, 0, newLen * sizeof(uintptr_t));

	size_t oldLen = self->length;
	struct Bucket **oldData = self->data;

	for (size_t i = 0; i < oldLen; i++) {
		struct Bucket *b = oldData[i];
		while (b) {
			int j = get_place(newLen, b->hash);
			struct Bucket *next = b->next;

			b->next = newData[j];
			newData[j] = b;

			b = next;
		}
	}

	self->data = newData;
	self->length = newLen;

	free(oldData);
}


uint32_t TsHashMap_times33(const uint8_t *data, size_t size) {
	uint32_t hash = 0;

	while (size) {
		hash = 33 * hash + *data++;
		size--;
	}

	return hash;

}

uint32_t TsHashMap_sdbm(const uint8_t *data, size_t size) {
	uint32_t hash = 0;

	while(size) {
		hash = *data++ + (hash << 6) + (hash << 16) - hash;
		size--;
	}

	return hash;
}

TsHashMap TsHashMap_init(size_t key, size_t val) {
	struct TsHashMap *hashMap = malloc(sizeof(*hashMap));

	hashMap->data = malloc(16 * sizeof(uintptr_t));
	memset(hashMap->data, 0, 16 * sizeof(uintptr_t));
	hashMap->capacity = 0;
	hashMap->length = 16;

	hashMap->key_size = key;
	hashMap->val_size = val;

	MAP_LOCK_INIT(hashMap);

	return hashMap;
}

void TsHashMap_destroy(TsHashMap self) {
	assert(self);

	MAP_LOCK_WRITE(self);

	size_t length = self->length;
	size_t capacity = self->capacity;

	struct Bucket **data = self->data;

	for (size_t i = 0; i < length && capacity; i++) {
		struct Bucket *next;
		for (struct Bucket *b = data[i]; b; b = next) {
			next = b->next;
			free(b);
			capacity--;
		}
	}

	free(self->data);

	MAP_LOCK_DESTROY(self);

	free(self);

}

size_t TsHashMap_size(TsHashMap self) {
	return self->capacity;
}

bool TsHashMap_get(TsHashMap self, void *key, void *val) {
	assert(self);
	assert(key);

	MAP_LOCK_READ(self);

	uint32_t hash = TsHashMap_times33(key, self->key_size);

	bool found = false;
	struct Bucket *bucket = get_bucket(self, hash, key);
	if (bucket) {
		memcpy(val, bucket->val, self->val_size);
		found = true;
	}

	MAP_LOCK_UNLOCK(self);

	return found;
}

void *TsHashMap_getAll(TsHashMap self, size_t *out_count) {
	assert(self);

	MAP_LOCK_READ(self);

	size_t length = self->length;
	size_t capacity = self->capacity;

	if (capacity == 0) {
		return NULL;
	}

	uint8_t *buffer = malloc(capacity * self->val_size);
	size_t count = 0;

	struct Bucket **data = self->data;

	for (size_t i = 0; i < length && count < capacity; i++) {
		struct Bucket *next;
		for (struct Bucket *b = data[i]; b; b = next) {
			next = b->next;
			memcpy(buffer + self->val_size * count, b->val, self->val_size);
			count++;
		}
	}

	*out_count = count;

	MAP_LOCK_UNLOCK(self);

	return buffer;
}

bool TsHashMap_contains(TsHashMap self, void *key) {
	assert(self);
	assert(key);

	MAP_LOCK_READ(self);

	uint32_t hash = TsHashMap_times33(key, self->key_size);
	struct Bucket *bucket = get_bucket(self, hash, key);
	bool result = bucket != NULL;

	MAP_LOCK_UNLOCK(self);

	return result;
}

bool TsHashMap_add(TsHashMap self, void *key, void *val) {
	assert(self);
	assert(key);
	assert(val);

	MAP_LOCK_WRITE(self);

	if ((float)self->capacity / (float)self->length >= CAPACITY_PEAK) {
		expand_map(self);
	}

	bool found = false;
	uint32_t hash = TsHashMap_times33(key, self->key_size);
	size_t index = get_place(self->length, hash);

	for (struct Bucket *b = self->data[index]; b; b = b->next) {
		if (bucket_equals(self, b, hash, key)) {
			memcpy(b->val, val, self->val_size);
			found = true;
		}
	}

	if (!found) {
		struct Bucket *bucket = bucket_init(self->key_size, self->val_size);
		memcpy(bucket->key, key, self->key_size);
		memcpy(bucket->val, val, self->val_size);
		bucket->hash = hash;
		bucket->next = self->data[index];
		self->data[index] = bucket;
		self->capacity++;
	}

	MAP_LOCK_UNLOCK(self);

	return found;
}

bool TsHashMap_replace(TsHashMap self, void *key, void *val) {
	assert(self);
	assert(key);
	assert(val);

	MAP_LOCK_WRITE(self);

	uint32_t hash = TsHashMap_times33(key, self->key_size);

	bool found = false;
	struct Bucket *bucket = get_bucket(self, hash, key);
	if (bucket) {
		memcpy(bucket->val, val, self->val_size);
		found = true;
	}

	MAP_LOCK_UNLOCK(self);

	return found;
}

bool TsHashMap_remove(TsHashMap self, void *key) {
	assert(self);
	assert(key);

	MAP_LOCK_WRITE(self);

	bool found = false;
	uint32_t hash = TsHashMap_times33(key, self->key_size);
	struct Bucket *bucket = steal_bucket(self, hash, key);

	if (bucket) {
		free(bucket);
		self->capacity--;
		found = true;
	}

	MAP_LOCK_UNLOCK(self);

	return found;
}

bool TsHashMap_steal(TsHashMap self, void *key, void *val) {
	assert(self);
	assert(key);

	MAP_LOCK_WRITE(self);

	bool found = false;
	uint32_t hash = TsHashMap_times33(key, self->key_size);
	struct Bucket *bucket = steal_bucket(self, hash, key);

	if (bucket) {
		memcpy(val, bucket->val, self->val_size);
		free(bucket);
		self->capacity--;
		found = true;
	}

	MAP_LOCK_UNLOCK(self);

	return found;
}


