#ifndef ARRAY_H_
#define ARRAY_H_

#include <stdbool.h>

typedef struct Array *Array;

Array Array_init(size_t size, size_t itemSize);
void Array_destroy(Array list);
void Array_clear(Array self);

void *Array_getData(Array self);
void *Array_getDataChecked(Array self, int index);

int Array_length(Array self);
bool Array_isEmpty(Array self);
bool Array_checkType(Array self, size_t itemSize);

void Array_add(Array self, void *item);

void Array_remove(Array self, int index);
void Array_remove_fast(Array self, int index);

int Array_indexOf(Array self, void *item, bool (*equals)(void *element, void *item));
bool Array_contains(Array self, void *item, bool (*equals)(void *element, void *item));

void Array_foreach(Array self, void (*func)(void *));
void Array_foreach_arg(Array self, void (*func)(void *, void *), void *arg);

#define ARRAY_GET(self, type, i) (((type *)Array_getDataChecked(self, i))[i])

#endif /* ARRAY_H_ */
