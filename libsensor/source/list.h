#ifndef LIST_H_
#define LIST_H_

#include <stdbool.h>

typedef struct List_item *List_iterator;
typedef struct List_s *List_t;


List_t list_init();
void list_destroy(List_t list);

/*---------------------iterations----------------------*/
List_iterator list_iterator(List_t list);
List_iterator list_iterator_back(List_t list);
void *list_next(List_t list, List_iterator current);
void *list_prev(List_t list, List_iterator current);
bool list_hasNext(List_t list, List_iterator current);
bool list_hasPrev(List_t list, List_iterator current);

/*-------------------content managment-----------------*/
void list_addTop(List_t list, void *data);
void list_addBot(List_t list, void *data);
void *list_removeTop(List_t list);
void *list_removeBot(List_t list);
void *list_removeFound(List_t list, void *data, size_t size);
void *list_removeFound_r(List_t list, void *data, int (*comparator)(void *, void *));


/*-------------------misc-----------------------*/
void list_map(List_t list, void (*func)(void *));
void *list_find(List_t list, void *data, size_t size);
void *list_find_r(List_t list, void *data, int (*comparator)(void *, void *));
void *list_toArray(List_t list);



#endif /* LIST_H_ */
