#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "arraylist.h"

#define RETURN_IF_FAIL(x) if (!x) return
#define RETURN_IF_FAILV(x,v) if (!x) return v

#define LAST_INDEX(self, i) (self->length - 1 == i)
#define POINTER void *

struct ArrayList {
	int length;
	size_t size;

	void (*destroyer) (void *);

	void **data;
};

static void moveData(void **data, int to, int from, int length) {
	memmove(&data[to], &data[from], (length - to) * sizeof(POINTER));
}

static void ensureSize(ArrayList self, int more) {
	int newLength = self->length + more;
	int oldSize = self->size;
	if (newLength > oldSize) {
		int newSize = oldSize * 3 / 2 + 1;
		if (newSize < newLength) {
			newSize = newLength;
		}
		self->data = realloc(self->data, newSize * sizeof(POINTER));
		self->size = newSize;
	}
}

ArrayList ArrayList_init(size_t size, ArrayList_destroyer func) {
	if (size < 1) {
		size = 10;
	}

	ArrayList list = malloc(sizeof(*list));
	list->data = malloc(size * sizeof(void *));
	list->length = 0;
	list->size = size;
	list->destroyer = func;

	return list;

}

static bool checkLength(ArrayList self, int index) {
	if (index < 0 || index > self->length) {
		return false;
	}
	return true;
}

void ArrayList_destroy(ArrayList self) {
	if (self->destroyer) {
		ArrayList_foreach(self, self->destroyer);
	}
	free(self->data);
	free(self);
}

void ArrayList_clear(ArrayList self) {
	if (self->destroyer) {
		ArrayList_foreach(self, self->destroyer);
	}
	self->length = 0;
}


ArrayList ArrayList_copy(ArrayList self) {
	ArrayList list = malloc(sizeof(*list));

	list->data = ArrayList_getDataCopy(self);

	/* use all allocated size if array is empty */
	size_t len;
	if (self->length != 0) {
		len = self->length;
	} else {
		len = self->size;
	}

	list->length = len;
	list->size = len;

	list->destroyer = 0; /* data will be cleaned only with parent */

	return list;
}

void **ArrayList_getData(ArrayList self) {
	return self->data;
}

void **ArrayList_getDataCopy(ArrayList self) {
	/* use all allocated size if array is empty */
	size_t len;
	if (self->length != 0) {
		len = self->length;
	} else {
		len = self->size;
	}

	void **dataCopy = malloc(len * sizeof(POINTER));
	memcpy(dataCopy, self->data, len * sizeof(POINTER));
	return dataCopy;
}

void *ArrayList_get(ArrayList self, int index) {
	assert(index >= 0);
	assert(index < self->length);
	RETURN_IF_FAILV(checkLength(self, index), NULL);
	return self->data[index];
}

int ArrayList_length(ArrayList self) {
	return self->length;
}

bool ArrayList_isEmpty(ArrayList self) {
	return self->length == 0;
}

void ArrayList_add(ArrayList self, void *item) {
	ensureSize(self, 1);
	self->data[self->length] = item;
	self->length++;
}

void ArrayList_remove(ArrayList self, int index) {
	RETURN_IF_FAIL(checkLength(self, index));

	if (self->destroyer != NULL)
		self->destroyer(self->data[index]);


	if (!LAST_INDEX(self, index))
		moveData(self->data, index, index + 1, self->length - index);

	self->length--;
}

void ArrayList_remove_fast(ArrayList self, int index) {
	RETURN_IF_FAIL(checkLength(self, index));

	if (self->destroyer != NULL)
		self->destroyer(self->data[index]);

	if (!LAST_INDEX(self, index))
		self->data[index] = self->data[self->length - 1];

	self->length--;
}

void *ArrayList_steal(ArrayList self, int index) {
	RETURN_IF_FAILV(checkLength(self, index), NULL);

	void *temp = self->data[index];

	if (!LAST_INDEX(self, index))
		moveData(self->data, index, index + 1, self->length - index);

	self->length--;

	return temp;
}

void *ArrayList_steal_fast(ArrayList self, int index) {
	RETURN_IF_FAILV(checkLength(self, index), NULL);

	void *temp = self->data[index];

	if (!LAST_INDEX(self, index))
		self->data[index] = self->data[self->length - 1];

	self->length--;

	return temp;
}

static bool equals_by_link(void *a, void *b) {
	return a == b ? true : false;
}

void *ArrayList_find(ArrayList self, void *item, bool (*equals)(void *element, void *item)) {
	assert(item);

	if (equals == NULL) {
		equals = equals_by_link;
	}

	void *result = NULL;
	for (int i = 0; i < self->length; i++) {
		if (equals(self->data[i], item)) {
			result = self->data[i];
			break;
		}
	}

	return result;
}

int ArrayList_indexOf(ArrayList self, void *item, bool (*equals)(void *element, void *item)) {
	assert(item);

	if (equals == NULL) {
		equals = equals_by_link;
	}

	int result = -1;
	for (int i = 0; i < self->length; i++) {
		if (equals(self->data[i], item)) {
			result = i;
			break;
		}
	}

	return result;
}

bool ArrayList_contains(ArrayList self, void *item, bool (*equals)(void *element, void *item)) {
	assert(item);
	return ArrayList_find(self, item, equals) != NULL;
}

void ArrayList_foreach(ArrayList self, void (*func)(void *)) {
	for (int i = 0; i < self->length; i++) {
		func(self->data[i]);
	}
}

void ArrayList_foreach_arg(ArrayList self, void (*func)(void *, void *), void *arg) {
	for (int i = 0; i < self->length; i++) {
		func(self->data[i], arg);
	}
}

void ArrayList_qsort(ArrayList self, int (*cmp)(const void *, const void *)) {
	qsort(self->data, self->length, sizeof(POINTER), cmp);
}

