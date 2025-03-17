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

bool map_search(map_t *map, void *key, void *value)
{
    if (!map || !key)
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

    uint32_t hash = hash_murmur3_32(key, map->key_size) & (arr_len - 1); // we ensure that arr_len is always a power of two
    uint32_t original_hash = hash;

    map_node_t node;

    while (true)
    {
        if (!dyn_arr_get(arr, hash, &node))
        {
            // this means the dynamic array node containing the index hash is not allocated
            // this is possible only if the current key is not present in the table since we never actually
            // destroy any map_node or dynamic array node
            return false;
        }

        if (!memcmp(node.key, key, map->key_size))
        {
            // the keys are equal
            if (!memcpy(value, node.value, map->value_size))
            {
                return false;
            }

            return true;
        }
    }

    return false;
}

// we don't actually remove the map_node; we just mark it as free
bool map_remove(map_t *map, void *key)
{
    if (!map || !key)
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

    uint32_t hash = hash_murmur3_32(key, map->key_size) & (arr_len - 1);
    uint32_t original_hash = hash;

    map_node_t node;

    while (true)
    {
        if (!dyn_arr_get(arr, hash, &node))
        {
            // this means the dynamic array node containing the index hash is not allocated
            // this is possible only if the current key is not present in the table since we never actually
            // destroy any map_node or dynamic array node
            return false;
        }

        if (!memcmp(node.key, key, map->key_size))
        {
            // the keys are equal
            node.is_empty = true;
            if (!dyn_arr_set(arr, hash, &node))
            {
                return false;
            }

            return true;
        }
    }

    return false;
}

static bool rehash(map_t *map)
{
    if (!map || map->allocated || map->arr)
    {
        return false;
    }

    stack_t *allocated = map->allocated;
    dyn_arr_t *arr = map->arr;
    size_t alloc_len = allocated->stack_size;

    stack_t *new_alloc = stack_create(allocated->data_size);
    if (!new_alloc)
    {
        return false;
    }
    map->allocated = new_alloc;

    map_node_t map_node;
    size_t hash_node_index;

    for (size_t index = 0; index < alloc_len; index++)
    {
        if (!stack_pop(allocated, &hash_node_index))
        {
            stack_delete(new_alloc);
            return false;
        }

        if (!dyn_arr_get(arr, hash_node_index, &map_node))
        {
            stack_delete(new_alloc);
            return false;
        }

        map_node.is_empty = true;
        if (!dyn_arr_set(arr, hash_node_index, &map_node))
        {
            stack_delete(new_alloc);
            free(map_node.key);
            free(map_node.value);
            return false;
        }

        if (!map_insert(map, map_node.key, map_node.value))
        {
            stack_delete(new_alloc);
            free(map_node.key);
            free(map_node.value);
            return false;
        }

        free(map_node.key);
        free(map_node.value);
    }

    return true;
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
        arr->len <<= 1U;
        if (!rehash)
        {
            arr->len >>= 1U;
            return false;
        }
    }

    arr_len = arr->len * MAX_NODE_SIZE; // maybe arr->len is updated

    uint32_t hash = hash_murmur3_32(key, map->key_size) & (arr_len - 1);

    map_node_t node;
    node.is_empty = false;

    while (true)
    {
        if (!dyn_arr_get(arr, hash, &node))
        {
            // the dynamic array node containing the index hash is not allocated yet

            node.key = malloc(map->key_size);
            if (!node.key)
            {
                return false;
            }

            if (!memcpy(node.key, key, map->key_size))
            {
                free(node.key);
                return false;
            }

            node.value = malloc(map->value_size);
            if (!node.value)
            {
                free(node.key);
                return false;
            }

            if (!memcpy(node.value, value, map->value_size))
            {
                free(node.key);
                free(node.value);
                return false;
            }

            // dyn_arr_set will copy the contents of the map_node node into the index hash
            // it will copy the pointers key and value and won't allocate them and save their values
            // so, we do this before inserting the node into the array
            // these will be freed when the map is destroyed
            if (!dyn_arr_set(arr, hash, &node))
            {
                free(node.key);
                free(node.value);
                return false;
            }

            if (!stack_push(allocated, &hash))
            {
                free(node.key);
                free(node.value);
                return false;
            }

            return true;
        }

        if (node.is_empty)
        {
            // empty place found
            // the found node's key and value pointers are allocated so we just copy values to them
            // reusing them in effect

            node.is_empty = false;

            if (!memcpy(node.key, key, map->key_size))
            {
                free(node.key);
                return false;
            }

            if (!memcpy(node.value, value, map->value_size))
            {
                free(node.key);
                free(node.value);
                return false;
            }

            if (!dyn_arr_set(arr, hash, &node))
            {
                free(node.key);
                free(node.value);
                return false;
            }

            if (!stack_push(allocated, &hash))
            {
                free(node.key);
                free(node.value);
                return false;
            }

            return true;
        }

        hash = (hash + 1) & (arr_len - 1); // linear probing
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

#define INIT_DYN_LEN (1U << 10) // can't be zero; must be a power of two

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