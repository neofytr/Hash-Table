#include "map.h"

typedef struct
{
    void *key;
    void *value;
    bool is_empty;
} map_node_t;

#define SEED 0x9747b28c
#define BUCKET_DOUBLING_CUTOFF (0.7)

static inline uint32_t hash_murmur3_32(const void *key, size_t key_size)
{
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = key_size / 4;
    uint32_t h = SEED;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t *blocks = (const uint32_t *)(data);
    for (int i = 0; i < nblocks; i++)
    {
        uint32_t k = blocks[i];
        k *= c1;
        k = (k << 15) | (k >> 17);
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }

    const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
    uint32_t k1 = 0;
    switch (key_size & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        h ^= k1;
    }

    h ^= key_size;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

// we don't actually remove the map_node; we just mark it as free
bool map_remove(map_t *map, void *key, void *value)
{
    if (!map || !key || !value)
    {
        return false;
    }

    if (!map->allocated || !map->arr)
    {
        return false;
    }

    stack_t *allocated = map->allocated;
    dyn_arr_t *arr = map->arr;

    size_t arr_len = arr->len * MAX_NODE_SIZE;

    uint32_t hash = hash_murmur3_32(key, map->key_size) % arr_len;
    uint32_t original_hash = hash;

    while (true)
    {
        map_node_t node;
        if (!dyn_arr_get(arr, hash, &node))
        {
            return false;
        }
        
        if (!memcmp())
    }
}

bool map_insert(map_t *map, void *key, void *value)
{
    if (!map || !key || !value)
    {
        return false;
    }

    if (!map->allocated || !map->arr)
    {
        return false;
    }

    stack_t *allocated = map->allocated;
    dyn_arr_t *arr = map->arr;

    size_t arr_len = arr->len * MAX_NODE_SIZE;

    if (allocated->stack_size >= BUCKET_DOUBLING_CUTOFF * arr_len)
    {
        // double the number of nodes in the dynamic array and rehash
    }

    arr_len = arr->len * MAX_NODE_SIZE; // maybe arr->len is updated

    uint32_t hash = hash_murmur3_32(key, map->key_size) % arr_len;

    while (true)
    {
        map_node_t node;
        node.is_empty = false;
        node.key = key;
        node.value = value;

        if (!dyn_arr_get(arr, hash, &node))
        {
            // the dynamic array node containing the index hash is not allocated yet
            // node is not changed as a result of this so no need to reset it
            if (!dyn_arr_set(arr, hash, value))
            {
                return false;
            }

            if (!stack_push(allocated, &hash))
            {
                return false;
            }

            return true;
        }

        if (node.is_empty)
        {
            // empty place found but node is reset
            node.is_empty = false;
            node.key = key;
            node.value = value;

            if (!dyn_arr_set(arr, hash, &node))
            {
                return false;
            }

            if (!stack_push(allocated, &hash))
            {
                return false;
            }
            return true;
        }

        hash = (hash + 1) % arr_len; // linear probing
    }

    return false;
}

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

#define INIT_DYN_LEN (1024) // can't be zero

    map->arr = dyn_arr_create(INIT_DYN_LEN, sizeof(map_node_t));
    if (!map->arr)
    {
        free(map);
        return NULL;
    }

    map->allocated = stack_create(sizeof(size_t));
    if (!map->allocated)
    {
        free(map->arr);
        free(map);
        return NULL;
    }

    map->key_size = key_size;
    map->value_size = value_size;

    return map;
}

bool map_destroy(map_t *map)
{
    if (!map || !map->allocated || !map->arr || !map->key_size || !map->value_size)
    {
        return false;
    }

    size_t index;
    while (!is_stack_empty(map->allocated))
    {
        if (!stack_pop(map->allocated, &index))
        {
            return false;
        }

        map_node_t node;
        if (!dyn_arr_get(map->arr, index, &node))
        {
            return false;
        }

        free(node.key);
        free(node.value);
    }

    if (!stack_delete(map->allocated))
    {
        return false;
    }

    dyn_arr_free(map->arr);
    return true;
}