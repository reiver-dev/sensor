#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "array.h"

#define RETURN_IF_FAIL(x) if (!x) return
#define RETURN_IF_FAILV(x,v) if (!x) return v

#define LAST_INDEX(self, i) (self->length - 1 == i)



struct Array {
	int length;
	size_t size;
	size_t itemSize;
	void **data;
};

static void moveData(void **data, size_t itemSize, int to, int from, int length) {
	memmove(&data[to], &data[from], (length - to) * sizeof(itemSize));
}

static void ensureSize(Array self, int more) {
	int newLength = self->length + more;
	int oldSize = self->size;
	if (newLength > oldSize) {
		int newSize = oldSize * 3 / 2 + 1;
		if (newSize < newLength) {
			newSize = newLength;
		}
		self->data = realloc(self->data, newSize * sizeof(self->itemSize));
		self->size = newSize;
	}
}

Array Array_init(size_t size, size_t itemSize) {
	if (size < 1) {
		size = 10;
	}

	Array list = malloc(sizeof(*list));
	list->data = malloc(size * sizeof(void *));
	list->length = 0;
	list->size = size;
	list->itemSize = itemSize;

	return list;

}

static bool checkLength(Array self, int index) {
	if (index < 0 || index > self->length) {
		return false;
	}
	return true;
}

void Array_destroy(Array self) {
	free(self->data);
	free(self);
}

void Array_clear(Array self) {
	self->length = 0;
}

void *Array_getData(Array self) {
	return self->data;
}

void *Array_getDataChecked(Array self, int index) {
	assert(index >= 0);
	assert(index < self->length);
	RETURN_IF_FAILV(checkLength(self, index), NULL);
	return self->data;
}

int Array_length(Array self) {
	return self->length;
}

bool Array_isEmpty(Array self) {
	return self->length == 0;
}

bool Array_checkType(Array self, size_t itemSize) {
	return self->itemSize == itemSize;
}

void Array_add(Array self, void *item) {
	ensureSize(self, 1);
	memcpy(self->data + self->length * sizeof(self->itemSize), item, sizeof(self->itemSize));
	self->length++;
}

void Array_remove(Array self, int index) {
	RETURN_IF_FAIL(checkLength(self, index));

	if (!LAST_INDEX(self, index))
		moveData(self->data, self->itemSize, index, index + 1, self->length - index);

	self->length--;
}

void Array_remove_fast(Array self, int index) {
	RETURN_IF_FAIL(checkLength(self, index));

	if (!LAST_INDEX(self, index))
		self->data[index] = self->data[self->length - 1];

	self->length--;
}

int Array_indexOf(Array self, void *item, bool (*equals)(void *element, void *item)) {
	assert(item);

	int result = -1;

	if (equals == NULL) {
		for (int i = 0; i < self->length; i++) {
			if (equals(self->data[i], item)) {
				result = i;
				break;
			}
		}
	} else {
		for (int i = 0; i < self->length; i++) {
			if (memcmp(&(self->data[i]), &item, self->size)) {
				result = i;
				break;
			}
		}
	}

	return result;
}

bool Array_contains(Array self, void *item, bool (*equals)(void *element, void *item)) {
	assert(item);
	return Array_indexOf(self, item, equals) >= 0;
}

void Array_foreach(Array self, void (*func)(void *)) {
	for (int i = 0; i < self->length; i++) {
		func(self->data[i]);
	}
}

void Array_foreach_arg(Array self, void (*func)(void *, void *), void *arg) {
	for (int i = 0; i < self->length; i++) {
		func(self->data[i], arg);
	}
}


