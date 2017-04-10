#ifndef HASH_MAP_
#define HASH_MAP_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DICT_LIMIT 10

#define LIMIT 200
#define INCR 50
#define FACTOR 0.5

typedef struct dict_entry{
	char *key;
    void *value;
    struct dict_entry *next;
}dict_entry;

typedef struct dict{
	size_t size;
	size_t limit;
	dict_entry **bucket;
	dict_entry **rebucket;
}dict;

void init_hash_map(dict *dict);

void h_put(dict *dict,char *key, void *value);

long hash_code(char *key,int size);

void* h_get(dict *dict,char *key);

void h_remove(dict *dict,char *key);

void rehash(dict *dict);

size_t h_size(dict *dict);

void h_destory(dict *dict);

#endif
