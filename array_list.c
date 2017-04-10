#include "array_list.h"


void init_array_list_param(array_list *list,size_t limit){
	if(list == NULL){
		return;
	}
	list->limit = limit;
	list->length = 0;
	list->element = malloc(sizeof(void*) * list->limit);
}

void init_array_list(array_list *list){
	init_array_list_param(list,A_INIT_LIMIT);
}

size_t a_resize(array_list *list){
	if(list == NULL){
		return 0;
	}
	if(list->limit < A_LIMIT){
		return list->limit * (1 + A_FACTOR);
	}else{
		return list->limit + A_INCR;
	}
}

void re_store(array_list *list){
	list->re_element = list->element;
	size_t origin_limit = list->limit;
	size_t limit = a_resize(list);
	init_array_list_param(list,limit);
	size_t i;
	for(i=0;i<origin_limit;i++){
		a_add(list,list->re_element[i]);
	}
	free(list->re_element);
}

void a_add(array_list *list,void *element){
	if(list == NULL){
		return;
	}
	if(list->length >= list->limit){
		re_store(list);
	}
	list->element[list->length++] = element;
}

void a_remove(array_list *list,size_t index){
	if(list == NULL){
		return;
	}
	if(index<0 || list->length<=index){
		return;
	}
	void *removed = list->element[index];
	if(index == list->length - 1){
		list->element[index] = NULL;
		list->length--;
		if(removed != NULL){
			free(removed);
		}
		return;
	}
	int i;
	for(i=index;i<list->length;i++){
		if(i+1 < list->length){
			list->element[i] = list->element[i+1];
		}
	}
	list->element[list->length-1] = NULL;
	list->length--;
	if(removed != NULL){
		free(removed);
	}
}

void* a_get(array_list *list,size_t index){
	if(list == NULL || index >= list->length){
		return NULL;
	}
	return list->element[index];
}

size_t a_size(array_list *list){
	if(list == NULL){
		return 0;
	}
	return list->length;
}

void a_destroy(array_list *list){
	if(list == NULL){
		return;
	}
	int i;
	void *p;
	for(i=0;i<list->length;i++){
		p = list->element[i];
		free(p);
	}
	free(list);
}
