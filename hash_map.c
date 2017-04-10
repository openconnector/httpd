#include "hash_map.h"

void init_hash_map_param(dict *dict,size_t limit){
	dict->limit = limit;
	dict->size = 0;
	int i;
	dict->bucket = malloc(sizeof(dict_entry*) * dict->limit);
	for(i=0; i<dict->limit;i++){
		dict->bucket[i] = NULL;
	}
}

void init_hash_map(dict *dict){
	init_hash_map_param(dict,DICT_LIMIT);
}

dict_entry* create_dict_entry(char *key,char *value){
	dict_entry *entry = malloc(sizeof(dict_entry));
	entry->key = malloc(sizeof(char) * (strlen(key) + 1));
	//entry->key = key;
	strcpy(entry->key,key);
	entry->value = value;
	entry->next = NULL;
	return entry;
}

void h_put(dict *dic,char *key, void *value){
	if(dic == NULL || key == NULL){
		return;
	}
	if(dic->size >= dic->limit){
		rehash(dic);
	}
	long code = hash_code(key,dic->limit);
	dict_entry *entry = NULL;

	if(dic->bucket[code] == NULL){
		entry =create_dict_entry(key,value);
		dic->bucket[code] = entry;
		dic->size++;
	}else{
		dict_entry *de = dic->bucket[code];
		while(de){
			if(strcmp(key,de->key) == 0){//delete the element with the same key
				de->value = value;
				return;
			}
			if(de->next == NULL){
				entry =create_dict_entry(key,value);
				de->next = entry;
				dic->size++;
				return;
			}
			de = de->next;
		}
	}
}

void* h_get(dict *dict,char *key){
	if(dict == NULL || key == NULL){
		return NULL;
	}
	long code = hash_code(key,dict->limit);
	dict_entry *bucket = dict->bucket[code];
	if(bucket != NULL){
		while(bucket){
			if(strcmp((bucket)->key,key) == 0){
				return (bucket)->value;
			}else{
				bucket = (bucket)->next;
			}
		}
	}
	return NULL;
}

void h_remove(dict *dict,char *key){
	if(dict == NULL || key == NULL){
		return;
	}
	long code = hash_code(key,dict->limit);
	dict_entry *bucket = dict->bucket[code];
	if(bucket == NULL){
		return;
	}
	dict_entry *prev = bucket;
	while(bucket && strcmp(key,bucket->key) != 0){
		prev = bucket;
		bucket = bucket->next;
	}
	if(bucket == NULL){
		return;
	}
	if(prev != bucket){
		prev->next = bucket->next;
	}else{
		dict->bucket[code] = bucket->next;
	}
	//free(bucket->value);
	//free(bucket);
	dict->size--;
}

long hash_code(char *key,int size){
	long len = strlen(key);
	if(len == 0){
		return 0;
	}
	long code = 0;
	long i=0;
	for(i=0; i<len; i++){
		code += (char)(*key++);
	}
	if(code >= size){
		code = code % size;
	}
	return code;
}

size_t h_resize(dict *dict){
	size_t limit = dict->limit;
	if(limit < LIMIT){
		return (size_t)(limit * (FACTOR + 1));
	}else{
		return limit + INCR;
	}
}

void rehash(dict *dict){
	if(dict == NULL){
		return;
	}
	size_t origin_limit = dict->limit;
	size_t limit = h_resize(dict);
	if(limit == 0){
		return;
	}
	dict->rebucket = dict->bucket;
	init_hash_map_param(dict,limit);
	int i;
	for(i=0;i<origin_limit;i++){
		dict_entry *entry = dict->rebucket[i];
		while(entry != NULL){
			h_put(dict,entry->key,entry->value);
			entry = entry->next;
		}
	}
	free(dict->rebucket);
}

size_t h_size(dict *dict){
	if(dict == NULL){
		return 0;
	}
	return dict->size;
}

void h_destory(dict *dict){
	if(dict == NULL){
		return;
	}
	int i;
	dict_entry * entry;
	dict_entry * next;
	for(i=0;i<dict->limit;i++){
		entry = dict->bucket[i];
		while(entry != NULL){
			next = entry->next;
			if(entry->key != NULL){
				free(entry->key);
			}
			if(entry->value != NULL){
				free(entry->value);
			}
			free(entry);
			entry = next;
		}
	}
	free(dict);
}


