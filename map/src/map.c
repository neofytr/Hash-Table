#include "../inc/map.h"
#include <stdio.h>

static bool map_insert_rehash(map_t *map, void *key_ptr, void *value_ptr);
static bool rehash(map_t *map);

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

    size_t hash = (size_t)hash_murmur3_32(key, map->key_size) & (map->curr_max_len - 1);
    size_t original_hash = hash;

    map_node_t node;

    while (true)
    {
        if (!dyn_arr_get(arr, hash, &node))
        {
            return false;
        }

        if (node.is_empty)
        {
            return false;
        }

        if (!memcmp(node.key, key, map->key_size))
        {
            // the keys are equal - copy value only if the output parameter is not NULL
            if (value)
            {
                memcpy(value, node.value, map->value_size);
            }
            return true;
        }

        hash = (hash + 1) & (map->curr_max_len - 1); // linear probing
        if (hash == original_hash)
        {
            return false;
        }
    }

    return false;
}

// we don't actually remove the map_node; we just mark it as free and also free the corresponding key and value
bool map_remove(map_t *map, void *key)
{
    // we also don't remove this key from the allocated stack
    // so when we rehash, we should also check if the element from the allocated stack is not empty
    if (!map || !key)
    {
        return false;
    }

    if (!map->allocated || !map->arr)
    {
        return false;
    }

    dyn_arr_t *arr = map->arr;

    stack_t *alloc = map->allocated;

    stack_t *new_alloc = stack_create(alloc->data_size);
    if (!new_alloc)
    {
        return false;
    }

    size_t hash = (size_t)hash_murmur3_32(key, map->key_size) & (map->curr_max_len - 1);
    size_t original_hash = hash;

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

        if (node.is_empty)
        {
            // Empty slot means key is not in the map
            return false;
        }

        if (!memcmp(node.key, key, map->key_size))
        {
            // the keys are equal
            void *key_ptr = node.key;
            void *value_ptr = node.value;

            node.is_empty = true;
            node.key = NULL;
            node.value = NULL;

            // remove the found index from the allocated stack
            size_t elt;
            for (size_t index = 0; index < alloc->stack_size; index++)
            {
                if (!stack_pop(alloc, &elt))
                {
                    stack_delete(new_alloc);
                    return false;
                }

                if (elt != hash)
                {
                    if (!stack_push(new_alloc, &elt))
                    {
                        stack_delete(new_alloc);
                        return false;
                    }
                }
            }

            map->allocated = new_alloc;
            stack_delete(alloc);

            if (!dyn_arr_set(arr, hash, &node))
            {
                return false;
            }

            free(key_ptr);
            free(value_ptr);

            return true;
        }

        hash = (hash + 1) & (map->curr_max_len - 1); // linear probing
        if (hash == original_hash)
        {
            // we've checked all positions and didn't find the key
            return false;
        }
    }

    return false;
}

static bool rehash(map_t *map)
{
    if (!map || !map->allocated || !map->arr)
    {
        return false;
    }

    stack_t *old_allocated = map->allocated;
    dyn_arr_t *arr = map->arr;
    size_t len = old_allocated->stack_size;

    typedef struct
    {
        void *key;
        void *value;
    } key_value_pair;

    key_value_pair *pairs = malloc(len * sizeof(key_value_pair));
    if (!pairs)
    {
        return false;
    }

    size_t pair_count = 0;
    size_t hash_index;
    map_node_t node;

    for (size_t i = 0; i < len; i++)
    {
        if (!stack_pop(old_allocated, &hash_index))
        {
            free(pairs);
            return false;
        }

        if (!dyn_arr_get(arr, hash_index, &node))
        {
            free(pairs);
            return false;
        }

        if (!node.is_empty)
        {
            pairs[pair_count].key = node.key;
            pairs[pair_count].value = node.value;
            pair_count++;
        }
    }

    stack_t *new_allocated = stack_create(sizeof(size_t));
    if (!new_allocated)
    {
        free(pairs);
        return false;
    }

    map->allocated = new_allocated;

    for (size_t i = 0; i < pair_count; i++)
    {
        if (!map_insert_rehash(map, pairs[i].key, pairs[i].value))
        {
            for (size_t j = i; j < pair_count; j++)
            {
                free(pairs[j].key);
                free(pairs[j].value);
            }

            stack_delete(new_allocated);

            map->allocated = old_allocated;

            free(pairs);
            return false;
        }
    }

    stack_delete(old_allocated);

    free(pairs);
    return true;
}

bool map_insert_rehash(map_t *map, void *key_ptr, void *value_ptr)
{
    if (!map || !key_ptr || !value_ptr)
    {
        return false;
    }

    if (!map->allocated || !map->arr)
    {
        return false;
    }

    stack_t *allocated = map->allocated;
    dyn_arr_t *arr = map->arr;

    size_t hash = (size_t)hash_murmur3_32(key_ptr, map->key_size) & (map->curr_max_len - 1);
    size_t original_hash = hash;

    map_node_t node;

    while (true)
    {
        if (!dyn_arr_get(arr, hash, &node))
        {
            node.key = key_ptr;
            node.value = value_ptr;
            node.is_empty = false;

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

        if (node.is_empty)
        {
            node.key = key_ptr;
            node.value = value_ptr;
            node.is_empty = false;

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

        hash = (hash + 1) & (map->curr_max_len - 1);
        if (hash == original_hash)
        {
            return false;
        }
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

    if (allocated->stack_size >= BUCKET_DOUBLING_CUTOFF * map->curr_max_len)
    {
        // double the number of nodes (same as doubling the places in the array) in the dynamic array and rehash
        map->curr_max_len <<= 1U;
        if (!rehash(map))
        {
            map->curr_max_len >>= 1U;
            return false;
        }
        // critical, change the allocated variable after rehashing since the stack changes during rehashing
        // this was causing the  key pushed during rehashing to be lost
        // it was pushed into the wrong allocated table

        allocated = map->allocated;
    }

    size_t hash = (size_t)hash_murmur3_32(key, map->key_size) & (map->curr_max_len - 1);
    size_t original_hash = hash;

    map_node_t node;

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

            node.is_empty = false;

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
            // empty place found, but key and value pointers may not be allocated
            // need to allocate memory for key and value

            // if pointers are NULL, allocate new memory
            if (!node.key)
            {
                node.key = malloc(map->key_size);
                if (!node.key)
                {
                    return false;
                }
            }

            if (!node.value)
            {
                node.value = malloc(map->value_size);
                if (!node.value)
                {
                    // free key if it was just allocated
                    free(node.key);
                    return false;
                }
            }

            node.is_empty = false;

            if (!memcpy(node.key, key, map->key_size))
            {
                return false;
            }

            if (!memcpy(node.value, value, map->value_size))
            {
                return false;
            }

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

        // check if the key already exists
        if (!memcmp(node.key, key, map->key_size))
        {
            // update existing key's value
            if (!memcpy(node.value, value, map->value_size))
            {
                return false;
            }

            if (!dyn_arr_set(arr, hash, &node))
            {
                return false;
            }

            return true;
        }

        hash = (hash + 1) & (map->curr_max_len - 1); // linear probing
        if (hash == original_hash)
        {
            // Hash table is full (should not happen with rehashing)
            return false;
        }
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

    map_node_t default_node;
    default_node.is_empty = true;
    default_node.key = NULL;
    default_node.value = NULL;

    map->arr = dyn_arr_create(INIT_DYN_LEN, sizeof(map_node_t), &default_node);
    if (!map->arr)
    {
        free(map);
        return NULL;
    }

    map->allocated = stack_create(sizeof(size_t));
    if (!map->allocated)
    {
        dyn_arr_free(map->arr);
        free(map);
        return NULL;
    }

    map->key_size = key_size;
    map->value_size = value_size;
    map->curr_max_len = INIT_DYN_LEN;

    return map;
}

bool map_destroy(map_t *map)
{
    if (!map || !map->allocated || !map->arr || !map->key_size || !map->value_size)
    {
        return false;
    }

    size_t index;
    map_node_t node;
    size_t len = map->allocated->stack_size;

    for (size_t counter = 0; counter < len; counter++)
    {
        if (!stack_pop(map->allocated, &index))
        {
            break;
        }

        if (!dyn_arr_get(map->arr, index, &node))
        {
            break;
        }

        free(node.key);
        free(node.value);
    }

    if (!stack_delete(map->allocated))
    {
        return false;
    }

    dyn_arr_free(map->arr);
    free(map);
    return true;
}