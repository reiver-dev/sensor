/*
 * ts_hash_table.hpp
 *
 *  Created on: 31.08.2012
 *      Author: reiver
 */

#ifndef TS_HASH_TABLE_HPP_
#define TS_HASH_TABLE_HPP_

#include <cstdint>
#include <limits>

#include "threading.hpp"

template<typename KeyT, typename ValueT>
class TsHashTable {
private:
	typedef unsigned int hash_t;

	struct Bucket {
		hash_t hash;
		KeyT key;
		ValueT val;
		Bucket *next;

		Bucket(hash_t hash, const KeyT &key, const ValueT &val, Bucket *next) :
				hash(hash), key(key), val(val), next(next) {
			//
		}

	};

	static hash_t times33(const KeyT &t) {
		hash_t hash = 0;
		const char *data = (char *) &t;
		size_t size = sizeof(KeyT);

		while (size) {
			hash = 33 * hash + *data++;
			size--;
		}
		return hash;
	}

	static int spread_hash(int h) {
		h ^= (h << 15) ^ 0xcd7dcd7d;
		h ^= (h >> 10);
		h ^= (h <<  3);
		h ^= (h >>  6);
		h ^= (h <<  2) + (h << 14);
		h ^= (h >> 16);
		return h;
	}

	static size_t get_place(size_t len, hash_t hash) {
			return hash & (len - 1);
	}

	RwLock lock;
	struct Bucket **table;
	size_t capacity;
	size_t length;

	Bucket *get_bucket(hash_t hash, const KeyT &key) {
		size_t index = get_place(length, hash);
		struct Bucket *bucket;
		for (bucket = table[index]; bucket; bucket = bucket->next) {
			if (bucket->hash == hash && bucket->key == key)
				break;
		}
		return bucket;
	}

	Bucket *steal_bucket(hash_t hash, const KeyT &key) {
		size_t index = get_place(length, hash);

		struct Bucket *bucket;
		struct Bucket *previous = NULL;

		for (bucket = table[index]; bucket; bucket = bucket->next) {
			if (bucket->hash == hash && bucket->key == key) {
				if (previous) { // reorder previous
					previous->next = bucket->next;
				} else { // bucket is first
					table[index] = bucket->next;
				}
				break;
			}
			// remember it
			previous = bucket;
		}

		return bucket;
	}

	void expand_map() {
		size_t newLen = 2 * length;

		Bucket **newData = (Bucket **) malloc(newLen * sizeof(Bucket *));
		memset(newData, 0, newLen * sizeof(Bucket *));

		size_t oldLen = length;
		struct Bucket **oldData = table;

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

		table = newData;
		length = newLen;

		free(oldData);
	}


public:

	TsHashTable() : capacity(0), length(16) {
		table = (Bucket **) malloc(length * sizeof(Bucket *));
		memset(table, 0, length * sizeof(Bucket *));
	}

	~TsHashTable() {
		lock.lockWrite();

		size_t length = this->length;
		size_t capacity = this->capacity;

		struct Bucket **data = this->table;

		for (size_t i = 0; i < length && capacity; i++) {
			Bucket *next;
			for (struct Bucket *b = data[i]; b; b = next) {
				next = b->next;
				delete b;
				capacity--;
			}
		}

		free(table);
	}

	size_t getSize() {
		return capacity;
	}

	bool get(const KeyT &key, ValueT &value) {
		lock.lockRead();

		hash_t hash = spread_hash(times33(key));

		bool found = false;
		struct Bucket *bucket = get_bucket(hash, key);
		if (bucket) {
			value = bucket->val;
			found = true;
		}

		lock.release();

		return found;
	}

	bool contains(const KeyT &key) {
		lock.lockRead();

		hash_t hash = spread_hash(times33(key));
		struct Bucket *bucket = get_bucket(hash, key);
		bool result = bucket != NULL;

		lock.release();

		return result;
	}

	bool add(const KeyT &key, const ValueT &val) {
		lock.lockWrite();

		if ((float)capacity / (float)length >= 0.75) {
			expand_map();
		}

		bool found = false;
		hash_t hash = spread_hash(times33(key));
		size_t index = get_place(length, hash);

		for (struct Bucket *b = table[index]; b; b = b->next) {
			if (b->hash == hash && b->key == key) {
				b->val = val;
				found = true;
			}
		}

		if (!found) {
			Bucket *bucket = new Bucket(hash, key, val, table[index]);
			table[index] = bucket;
			capacity++;
		}

		lock.release();

		return found;
	}

	bool replace(const KeyT &key, ValueT &val) {

		lock.lockWrite();

		hash_t hash = spread_hash(times33(key));

		bool found = false;
		struct Bucket *bucket = get_bucket(hash, key);
		if (bucket) {
			bucket->val = val;
			found = true;
		}

		lock.release();

		return found;
	}

	bool remove(const KeyT &key) {
		lock.lockWrite();

		bool found = false;
		hash_t hash = spread_hash(times33(key));
		struct Bucket *bucket = steal_bucket(hash, key);

		if (bucket) {
			delete bucket;
			capacity--;
			found = true;
		}

		lock.release();

		return found;
	}

	bool steal(const KeyT &key, ValueT &val) {
		lock.lockWrite();

		bool found = false;
		hash_t hash = spread_hash(times33(key));
		struct Bucket *bucket = steal_bucket(hash, key);

		if (bucket) {
			val = bucket->val;
			delete bucket;
			capacity--;
			found = true;
		}

		lock.release();

		return found;
	}


};

#endif /* TS_HASH_TABLE_HPP_ */
