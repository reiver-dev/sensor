#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "list.h"

#define COMPARE(list, first, second) (list->comparator ? list->comparator(first, second) : mem)


struct List_item {
	struct List_item *next;
	struct List_item *prev;
	void *data;
};

struct List_s {
	struct List_item *first;
	struct List_item *last;

	int length;
	size_t size;

	void (*destroyer) (void *);
};


void *removeSingle(List_t list) {
	assert(list);
	assert(list->length == 1);

	void *result = list->first->data;

	free(list->first);
	list->first = NULL;
	list->last  = NULL;

	return result;

}



List_t list_init(){
	List_t list;
	list = malloc(sizeof(*list));
	list->first=0;
	list->last=0;
	list->length=0;
	list->destroyer = 0;
	return list;
}

void list_destroy(List_t list) {
	if (!list)
		return;

	assert(list);

	List_iterator iterator = list_iterator(list);
	while (list_hasNext(list, iterator)) {
		list->destroyer ? list->destroyer(iterator->data) : free(iterator->data);

		List_iterator tmp = iterator;
		iterator = iterator->next;
		free(tmp);
	}

	free(list);
	list = NULL;
}



/*---------------------iterations----------------------*/
List_iterator list_iterator(List_t list) {
	assert(list);
	return list->first;
}

List_iterator list_iterator_back(List_t list) {
	assert(list);
	return list->last;
}

void *list_next(List_t list, List_iterator current) {
	assert(list);
	assert(current);

	void *result = current->data;
	current = current->next;

	return result;
}

void *list_prev(List_t list, List_iterator current) {
	assert(list);
	assert(current);

	void *result = current->data;
	current = current->next;

	return result;
}

bool list_hasNext(List_t list, List_iterator current) {
	assert(list);
	assert(current);

	if (!current)
		return false;

	return current->next ? true : false;
}

bool list_hasPrev(List_t list, List_iterator current) {
	assert(list);

	if (current)
		return false;

	return current->prev ? true : false;
}



/*-------------------content managment-----------------*/
void list_addTop(List_t list, void *data) {
	assert(list);

	struct List_item *item = malloc(sizeof(*item));
	item->prev  = NULL;

	if (!list->first) {
		item->next  = NULL;
		list->last  = item;
	} else {
		item->next = list->first;
		list->first->prev = item;
	}

	list->first = item;
	list->length++;
}

void list_addBot(List_t list, void *data) {
	assert(list);

	struct List_item *item = malloc(sizeof(*item));
	item->next = NULL;

	if (!list->first) {
		list->first = item;
		item->prev  = NULL;
	} else {
		list->last->next = item;
		item->prev = list->last;
	}

	list->last  = item;

	list->length++;
}


void insertBefore(List_iterator current, void* data) {
	struct List_item *item = malloc(sizeof(*item));
	item->data = data;
	item->next = current;
	item->prev = current->prev;

	current->prev->next = item;
	current->prev = item;
}

void list_addSorted_asc(List_t list, void *data, int (*comparator)(void *, void *)) {
	assert(list);
	assert(data);
	assert(comparator);

	if(!list->length || comparator(list->first, data) > 0) {
		list_addTop(list, data);
	} else if (comparator(list->last, data) < 0) {
		list_addBot(list, data);

	} else if (comparator(list->first, data) < 0 && comparator(list->last, data) > 0) {

		List_iterator iterator = list_iterator(list);

		while (list_hasNext(list, iterator)) {
			if (!comparator(iterator->data, data)) {
				return;
			}
			if (comparator(iterator->data, data) > 0) {
				insertBefore(iterator, data);
			}

			iterator = iterator->next;
		}

	}

}


void *list_removeTop(List_t list) {
	assert(list);
	if (!list->length)
		return NULL;

	if (list->length == 1)
		return removeSingle(list);


	struct List_item *item = list->first;
	list->first = list->first->next;
	list->length--;

	if (list->length == 1)
		list->first = list->last;

	void* result = item->data;
	free(item);
	return result;
}

void *list_removeBot(List_t list) {
	assert(list);
	if (!list->length)
		return NULL;

	if (list->length == 1)
		return removeSingle(list);


	struct List_item *item = list->last;
	list->last = list->last->prev;
	list->length--;

	if (list->length == 1) {
		list->first = list->last;
	}

	void* result = item->data;
	free(item);
	return result;
}

void *removeMiddle(List_t list, struct List_item *item) {
	if (!list->length)
		return NULL;

	if(item == list->first) {
		return list_removeTop(list);
	} else if (item == list->last) {
		return list_removeBot(list);
	}


	//previous
	item->prev->next = item->next;
	item->next->prev = item->prev;


	list->first = list->first->next;
	list->length--;

	if (list->length <= 1) {
		list->last = list->first;
	}

	void* result = item->data;
	free(item);
	return result;
}

void *list_removeFound(List_t list, void *data, size_t size) {
	assert(list);
	assert(data);

	List_iterator iterator = list_iterator(list);
	while (list_hasNext(list, iterator)) {
		if (!memcmp(iterator->data, data, size))
			return removeMiddle(list, iterator);

		iterator = iterator->next;
	}

	return NULL;
}

void *list_removeFound_r(List_t list, void *data, int (*comparator)(void *, void *)) {
	assert(list);
	assert(data);

	List_iterator iterator = list_iterator(list);
	while (list_hasNext(list, iterator)) {
		if (!comparator(iterator->data, data))
			return removeMiddle(list, iterator);

		iterator = iterator->next;
	}

	return NULL;
}


/*-------------------misc-----------------------*/


void list_map(List_t list, void (*func)(void *)) {
	assert(list);
	assert(func);

	List_iterator iterator = list_iterator(list);
	while(list_hasNext(list, iterator)) {
		void *data = list_next(list, iterator);
		func(data);
	}

}

void *list_find(List_t list, void *data, size_t size) {
	assert(list);
	assert(data);

	List_iterator iterator = list_iterator(list);
	while (list_hasNext(list, iterator)) {
		void *current = list_next(list, iterator);
		if (!memcmp(current, data, size)) {
			return current;
		}
	}

	return NULL;


}

void *list_find_r(List_t list, void *data, int (*comparator)(void *, void *)) {
	assert(list);
	assert(data);
	assert(comparator);

	List_iterator iterator = list_iterator(list);
	while (list_hasNext(list, iterator)) {
		void *current = list_next(list, iterator);
		if (!comparator(current, data))
			return current;
	}

	return NULL;
}

void *list_toArray(List_t list) {
	assert(list);
	if (list->length)
		return NULL;

	void **array = malloc(list->length * sizeof(void*));

	List_iterator iterator = list_iterator(list);
	for(int i=0; list_hasNext(list, iterator); i++) {
		void *current = list_next(list, iterator);
		array[i] = current;
	}

	return array;
}
