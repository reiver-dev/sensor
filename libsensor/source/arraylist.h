#ifndef ARRAYLIST_H_
#define ARRAYLIST_H_

#include <stdlib.h>
#include <stdbool.h>

#define ARRAYLIST_GET(self, type, index) ((type) ArrayList_get(self, index))

typedef struct ArrayList *ArrayList;
typedef void (*ArrayList_destroyer)(void *);

ArrayList ArrayList_init(size_t size, ArrayList_destroyer func);
void ArrayList_destroy(ArrayList list);
void ArrayList_clear(ArrayList self);
ArrayList ArrayList_copy(ArrayList self);

void **ArrayList_getData(ArrayList self);
void **ArrayList_getDataCopy(ArrayList self);
void *ArrayList_get(ArrayList self, int index);

int ArrayList_length(ArrayList self);
bool ArrayList_isEmpty(ArrayList self);

void ArrayList_add(ArrayList self, void *item);

void ArrayList_remove(ArrayList self, int index);
void ArrayList_remove_fast(ArrayList self, int index);
void *ArrayList_steal(ArrayList self, int index);
void *ArrayList_steal_fast(ArrayList self, int index);

void *ArrayList_find(ArrayList self, void *item, bool (*equals)(void *element, void *item));
int ArrayList_indexOf(ArrayList self, void *item, bool (*equals)(void *element, void *item));
bool ArrayList_contains(ArrayList self, void *item, bool (*equals)(void *element, void *item));

void ArrayList_foreach(ArrayList self, void (*func)(void *));
void ArrayList_foreach_arg(ArrayList self, void (*func)(void *, void *), void *arg);
void ArrayList_qsort(ArrayList self, int (*cmp)(const void *, const void *));

#endif /* ARRAYLIST_H_ */
