#ifndef MAP_H
#define MAP_H

#include "dyn_arr/inc/dyn_arr.h"
#include "stack.h"

typedef struct
{
    dyn_arr_t *arr;
    stack_t *allocated;
    size_t key_size;
    size_t value_size;
} map_t;

bool map_insert(map_t *map, void *key, void *value);
bool map_remove(map_t *map, void *key);
bool map_search(map_t *map, void *key, void *value);
map_t *map_create(size_t key_size, size_t value_size); // key and value size in bytes
bool map_destroy(map_t *map);

#endif
