#include "map.h"

typedef struct
{
    void *key;
    void *value;
    bool is_empty;
} map_node_t;

typedef struct
{
    size_t node_no;
    size_t node_index;
} map_allocated_t;

map_t *map_create(size_t key_size, size_t value_size)
{
    if (!key_size || !value_size)
    {
        return NULL;
    }

    map_t *map = (map_t *)malloc(sizeof(map_t));
    if (!map)
    {
        return NULL;
    }

    map->arr = dyn_arr_create(0, sizeof(map_node_t));
    if (!map->arr)
    {
        return NULL;
    }

    map->key_size = key_size;
    map->value_size = value_size;

    return map;
}

bool map_destroy(map_t *map)
{
    if (!map)
    {
        return false;
    }
}