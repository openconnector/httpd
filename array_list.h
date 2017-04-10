#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define A_INIT_LIMIT 10
#define A_LIMIT 200
#define A_FACTOR 0.5
#define A_INCR 50

typedef struct array_list{
	size_t length;
	size_t limit;
	void **element;
	void **re_element;
}array_list;

void init_array_list(array_list *list);

void a_add(array_list *list,void *element);

void a_remove(array_list *list,size_t index);

void* a_get(array_list *list,size_t index);

size_t a_size(array_list *list);

void a_destroy(array_list *list);

#endif /* ARRAY_LIST_H_ */
