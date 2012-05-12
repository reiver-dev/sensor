#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>


#include "hashmap.h"

#define EXPAND_FACTOR 2
#define CAPACITY_PEAK 0.75

struct Bucket {
	void *key;
	void *val;
	uint32_t hash;
	struct Bucket *next;
};

struct HashMap {
	struct Bucket **data;

	size_t length;
	size_t capacity;

	HashMapHash hashf;
	HashMapEquals equals;

	HashMapDestroyer destroy_key;
	HashMapDestroyer destroy_val;
};

static struct Bucket *bucket_init(uint32_t hash, void *key, void *val) {
	struct Bucket *bucket = malloc(sizeof(*bucket));

	bucket->hash = hash;
	bucket->key  = key;
	bucket->val  = val;
	bucket->next = NULL;

	return bucket;
}

static void bucket_destroy(HashMap self, struct Bucket *bucket) {

	if (self->destroy_key) {
		self->destroy_key(bucket->key);
	}

	if (self->destroy_val) {
		self->destroy_val(bucket->val);
	}

	free(bucket);
}

static bool bucket_equals(HashMap self, struct Bucket *bucket, uint32_t hash, void *key) {
	bool f1 = bucket->hash == hash;
	bool f2 = self->equals(bucket->key, key);
	if (f1 && f2) {
		return true;
	}
	return false;
}

static size_t get_place(size_t len, uint32_t hash) {
	return hash & (len - 1);
}

static struct Bucket *get_bucket(HashMap self, uint32_t hash, void *key) {
	size_t index = get_place(self->length, hash);

	struct Bucket *bucket = self->data[index];
	if (bucket == NULL || bucket->next == NULL) {
		return bucket;
	}

	/* collision */
	while (bucket != NULL) {
		if (bucket_equals(self, bucket, hash, key)) {
			break;
		}
		bucket = bucket->next;
	}

	return bucket;
}

static struct Bucket *steal_bucket(HashMap self, uint32_t hash, void *key) {
	size_t index = get_place(self->length, hash);

	struct Bucket *bucket = self->data[index];
	if (bucket == NULL || bucket->next == NULL) {
		self->data[index] = NULL;
		return bucket;
	}

	/* collision */
	struct Bucket *prev = NULL;
	while (bucket != NULL) {
		if (bucket_equals(self, bucket, hash, key)) {
			if (prev && bucket->next) {
				prev = bucket->next;
			}
			break;
		}
		prev = bucket;
		bucket = bucket->next;
	}

	return bucket;
}


static void expand_map(HashMap self) {
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


uint32_t HashMap_times33(const uint8_t *data, size_t size) {
	uint32_t hash = 0;

	while (size) {
		hash = 33 * hash + *data++;
		size--;
	}

	return hash;

}

uint32_t HashMap_sdbm(const uint8_t *data, size_t size) {
	uint32_t hash = 0;

	while(size) {
		hash = *data++ + (hash << 6) + (hash << 16) - hash;
		size--;
	}

	return hash;
}


static uint32_t hash_int32(const void *data) {
	return HashMap_sdbm(data, sizeof(uint32_t));
}

static uint32_t hash_string(const void *data) {
	const uint8_t *str = data;
	uint32_t hash = 0;

	while (*str)
		hash = *str++ + (hash << 6) + (hash << 16) - hash;

	return hash;
}

static bool equals_int32(const void *i1, const void *i2) {
	return *(uint32_t *)i1 == *(uint32_t *)i2;
}

static bool equals_string(const void *s1, const void *s2) {
	return strcmp((char *)s1, (char *)s2) == 0;
}

HashMap HashMap_init(HashMapHash hash, HashMapEquals equals, HashMapDestroyer key, HashMapDestroyer value) {
	struct HashMap *hashMap = malloc(sizeof(*hashMap));

	hashMap->data = malloc(16 * sizeof(uintptr_t));
	memset(hashMap->data, 0, 16 * sizeof(uintptr_t));
	hashMap->capacity = 0;
	hashMap->length = 16;

	hashMap->hashf = hash;
	hashMap->equals = equals;
	hashMap->destroy_key = key;
	hashMap->destroy_val = value;

	return hashMap;
}

HashMap HashMap_initString(HashMapDestroyer key, HashMapDestroyer value) {
	return HashMap_init(hash_string, equals_string, key, value);
}


HashMap HashMap_initInt32(HashMapDestroyer key, HashMapDestroyer value) {
	return HashMap_init(hash_int32, equals_int32, key, value);
}

void HashMap_destroy(HashMap self) {
	assert(self);

	size_t length = self->length;
	size_t capacity = self->capacity;
	struct Bucket **data = self->data;
	for (size_t i = 0; i < length && capacity; i++) {
		struct Bucket *next;
		for (struct Bucket *b = data[i]; b; b = next) {
			next = b->next;
			bucket_destroy(self, b);
			capacity--;
		}
	}

	free(self->data);
	free(self);

}

size_t HashMap_size(HashMap self) {
	return self->capacity;
}

void **HashMap_getKeys(HashMap self, void **keys) {

	size_t length = self->length;
	size_t capacity = self->capacity;
	struct Bucket **data = self->data;

	if (capacity == 0) {
		return NULL;
	}

	if (keys == NULL) {
		keys = malloc((capacity + 1) * sizeof(uintptr_t));
		memset(keys, 0, (capacity + 1) * sizeof(uintptr_t));
	}

	size_t index = 0;
	for (size_t i = 0; i < length && capacity; i++) {
		struct Bucket *next;
		for (struct Bucket *b = data[i]; b; b = next) {
			next = b->next;
			keys[index] = b->key;
			index++;
			capacity--;
		}
	}

	return keys;
}
void **HashMap_getValues(HashMap self, void **vals) {

	size_t length = self->length;
	size_t capacity = self->capacity;
	struct Bucket **data = self->data;

	if (capacity == 0) {
		return NULL;
	}

	if (vals == NULL) {
		vals = malloc((capacity + 1) * sizeof(uintptr_t));
		memset(vals, 0, (capacity + 1) * sizeof(uintptr_t));
	}

	size_t index = 0;
	for (size_t i = 0; i < length && capacity; i++) {
		struct Bucket *next;
		for (struct Bucket *b = data[i]; b; b = next) {
			next = b->next;
			vals[index] = b->val;
			index++;
			capacity--;
		}
	}

	return vals;
}
struct HashMapPair *HashMap_getPairs(HashMap self) {
	size_t length = self->length;
	size_t capacity = self->capacity;
	struct Bucket **data = self->data;

	if (capacity == 0) {
		return NULL;
	}

	struct HashMapPair *pairs = malloc((capacity + 1) * sizeof(*pairs));
	memset(pairs, 0, (capacity + 1) * sizeof(uintptr_t));

	size_t index = 0;
	for (size_t i = 0; i < length && capacity; i++) {
		struct Bucket *next;
		for (struct Bucket *b = data[i]; b; b = next) {
			next = b->next;

			pairs[index].key = b->key;
			pairs[index].value = b->val;
			index++;

			capacity--;
		}
	}

	return pairs;
}

void *HashMap_get(HashMap self, void *key) {
	assert(self);
	assert(key);

	uint32_t hash = self->hashf(key);

	struct Bucket *bucket = get_bucket(self, hash, key);

	if (bucket != NULL) {
		return bucket->val;
	} else {
		return NULL;
	}
}

bool HashMap_contains(HashMap self, void *key) {
	assert(self);
	assert(key);

	uint32_t hash = self->hashf(key);
	struct Bucket *bucket = get_bucket(self, hash, key);

	return bucket != NULL;

}

bool HashMap_add(HashMap self, void *key, void *val) {
	assert(self);
	assert(key);
	assert(val);

	if ((float)self->capacity / (float)self->length >= CAPACITY_PEAK) {
		expand_map(self);
	}

	uint32_t hash = self->hashf(key);
	size_t index = get_place(self->length, hash);

	for (struct Bucket *b = self->data[index]; b; b = b->next) {
		if (bucket_equals(self, b, hash, key)) {
			if (self->destroy_val)
				self->destroy_val(b->val);
			b->val = val;
			return false;
		}
	}

	struct Bucket *bucket = bucket_init(hash, key, val);

	bucket->next = self->data[index];
	self->data[index] = bucket;

	self->capacity++;

	return true;
}

void HashMap_addInt32(HashMap self, uint32_t key, void *val) {
	uint32_t *temp = malloc(sizeof(*temp));
	*temp = key;
	if (!HashMap_add(self, temp, val)) {
		free(temp);
	}
}

void HashMap_remove(HashMap self, void *key) {
	assert(self);
	assert(key);

	uint32_t hash = self->hashf(key);

	struct Bucket *bucket = steal_bucket(self, hash, key);
	if (bucket != NULL) {
		bucket_destroy(self, bucket);
		self->capacity--;
	}
}

void *HashMap_steal(HashMap self, void *key) {
	assert(self);
	assert(key);

	uint32_t hash = self->hashf(key);

	struct Bucket *bucket = steal_bucket(self, hash, key);
	if (bucket != NULL) {
		self->capacity--;
	}

	return bucket;

}


struct HashMapIndex HashMap_first(HashMap self) {
	struct HashMapIndex hmindex = {
			.pos = 0,
			.index = 0,
			.this = NULL
	};
	return hmindex;
}

bool HashMap_hasNext(HashMap self, struct HashMapIndex *index) {
	if (self->capacity && index->pos < self->capacity && index->index < self->length)
		return true;

	return false;
}

struct HashMapPair *HashMap_next(HashMap self, struct HashMapIndex *index) {

	size_t length = self->length;
	struct Bucket **data = self->data;

	if (!HashMap_hasNext(self, index))
		return NULL;

	struct Bucket *now = (struct Bucket *)index->this;
	if (index->this != NULL && now->next != NULL) {
		index->this = (struct HashMapPair *)now->next;

	} else {
		for (size_t i = index->index; i < length; i++) {
			if (data[i]) {
				index->index = i;
				index->this = (struct HashMapPair *)data[i];
				break;
			}
		}
	}

	index->pos++;
	return index->this;
}
