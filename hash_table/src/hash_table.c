#include "../inc/hash_table.h"

#include <string.h>

#define SEED 0x9747b28c
#define BUCKET_DOUBLING_CUTOFF (0.3)

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

hash_table_t *hash_table_create(size_t num_of_buckets, size_t key_size, size_t value_size)
{
    hash_table_t *table = malloc(sizeof(hash_table_t));
    if (!table)
        return NULL;

    table->num_of_buckets = num_of_buckets;
    table->buckets = calloc(num_of_buckets, sizeof(node_t *));
    if (!table->buckets)
    {
        free(table);
        return NULL;
    }

    table->value_size = value_size;
    table->key_size = key_size;
    table->free_nodes = NULL;
    table->num_of_nodes = 0;

    return table;
}

void hash_table_destroy(hash_table_t *table)
{
    if (!table)
        return;

    for (size_t i = 0; i < table->num_of_buckets; i++)
    {
        node_t *current = table->buckets[i];
        while (current)
        {
            node_t *next = current->next;
            free(current->key);
            free(current->value);
            free(current);
            current = next;
        }
    }

    node_t *current = table->free_nodes;
    while (current)
    {
        node_t *next = current->next;
        free(current->key);
        free(current->value);
        free(current);
        current = next;
    }

    free(table->buckets);
    free(table);
}

hash_table_t *hash_table_merge(hash_table_t **hash_table_arr, size_t len, hash_value_add add_value, size_t key_size, size_t value_size, size_t new_bucket_num)
{
    if (!hash_table_arr)
    {
        return NULL;
    }

    for (size_t index = 0; index < len; index++)
    {
        hash_table_t *table = hash_table_arr[index];
        if (!table || (table->key_size != key_size) || (table->value_size != value_size))
        {
            return NULL;
        }
    }

    hash_table_t *merged_table = hash_table_create(new_bucket_num, key_size, value_size);
    if (!merged_table)
    {
        return NULL;
    }

    uint8_t *value = (uint8_t *)malloc(value_size);
    if (!value)
    {
        hash_table_destroy(merged_table);
        return NULL;
    }

    uint8_t *new_val = (uint8_t *)malloc(value_size);
    if (!new_val)
    {
        free(value);
        hash_table_destroy(merged_table);
        return NULL;
    }

    for (size_t index = 0; index < len; index++)
    {
        hash_table_t *table = hash_table_arr[index];
        for (size_t counter = 0; counter < table->num_of_buckets; counter++)
        {
            node_t *curr = table->buckets[counter];
            while (curr)
            {
                if (!curr->is_free)
                {
                    if (!hash_table_search(merged_table, curr->key, (void *)value))
                    {
                        if (!hash_table_insert(merged_table, curr->key, curr->value))
                        {
                            free(value);
                            free(new_val);
                            hash_table_destroy(merged_table);
                            return NULL;
                        }
                    }
                    else
                    {
                        if (!add_value((void *)value, curr->value, (void *)new_val))
                        {
                            free(value);
                            free(new_val);
                            hash_table_destroy(merged_table);
                            return NULL;
                        }

                        if (!hash_table_insert(merged_table, curr->key, new_val))
                        {
                            free(value);
                            free(new_val);
                            hash_table_destroy(merged_table);
                            return NULL;
                        }
                    }
                }
                curr = curr->next;
            }
        }
    }

    free(value);
    free(new_val);
    return merged_table;
}

bool hash_table_resize(hash_table_t *table, size_t new_bucket_count)
{
    if (!table || new_bucket_count <= 0)
        return false;

    node_t **new_buckets = calloc(new_bucket_count, sizeof(node_t *));
    if (!new_buckets)
        return false;

    size_t old_bucket_count = table->num_of_buckets;
    node_t **old_buckets = table->buckets;

    // rehash all entries
    for (size_t i = 0; i < old_bucket_count; i++)
    {
        node_t *current = old_buckets[i];
        while (current)
        {
            node_t *next = current->next;

            if (!current->is_free)
            {
                // calculate new hash based on new bucket count
                unsigned long new_hash = hash_murmur3_32(current->key, table->key_size) % new_bucket_count;

                // insert at beginning of new bucket chain
                current->next = new_buckets[new_hash];
                new_buckets[new_hash] = current;
            }
            else
            {
                // handle free nodes
                current->next = table->free_nodes;
                table->free_nodes = current;
            }

            current = next;
        }
    }

    // update table structure
    free(old_buckets);
    table->buckets = new_buckets;
    table->num_of_buckets = new_bucket_count;

    return true;
}

bool hash_table_insert(hash_table_t *table, const void *key, const void *value)
{
    if (!table || !key || !value)
        return false;

    if (table->num_of_nodes >= BUCKET_DOUBLING_CUTOFF * table->num_of_buckets)
    {
        if (!hash_table_resize(table, table->num_of_buckets * 2))
        {
            // rehashing failed will lead to a performance degrade
        }
    }

    unsigned long hash = hash_murmur3_32(key, table->key_size) % table->num_of_buckets;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (!current->is_free && !memcmp(current->key, key, table->key_size))
        {
            memcpy(current->value, value, table->value_size);
            return true;
        }
        current = current->next;
    }

    node_t *new_node = NULL;
    if (table->free_nodes)
    {
        new_node = table->free_nodes;
        table->free_nodes = new_node->next;
    }
    else
    {
        new_node = malloc(sizeof(node_t));
        if (!new_node)
            return false;

        new_node->key = malloc(table->key_size);
        if (!new_node->key)
        {
            free(new_node);
            return false;
        }

        new_node->value = malloc(table->value_size);
        if (!new_node->value)
        {
            free(new_node->key);
            free(new_node);
            return false;
        }
    }

    memcpy(new_node->key, key, table->key_size);
    memcpy(new_node->value, value, table->value_size);

    new_node->next = table->buckets[hash];
    new_node->is_free = false;
    table->buckets[hash] = new_node;

    table->num_of_nodes++;

    return true;
}

// marks all the entries in the table as free
bool hash_table_clear(hash_table_t *table)
{
    if (!table)
    {
        return false;
    }

    for (size_t index = 0; index < table->num_of_buckets; index++)
    {
        node_t *curr = table->buckets[index];
        while (curr)
        {
            node_t *next = curr->next;

            if (!curr->is_free)
            {
                curr->is_free = true;
                curr->next = table->free_nodes;
                table->free_nodes = curr;
            }

            curr = next;
        }
        table->buckets[index] = NULL;
    }

    table->num_of_nodes = 0;
    return true;
}

// doesn't actually delete the entry, just marks it as free
// this is better since we can use the space allocated for some other entry
bool hash_table_delete(hash_table_t *table, const void *key)
{
    if (!table || !key)
        return false;

    unsigned long hash = hash_murmur3_32(key, table->key_size) % table->num_of_buckets;

    node_t *current = table->buckets[hash];
    node_t *prev = NULL;

    while (current)
    {
        if (!current->is_free && !memcmp(current->key, key, table->key_size))
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                table->buckets[hash] = current->next;
            }

            current->is_free = true;
            current->next = table->free_nodes;
            table->free_nodes = current;

            table->num_of_nodes--;

            return true;
        }

        prev = current;
        current = current->next;
    }

    return false;
}

bool hash_table_search(hash_table_t *table, const void *key, void *value)
{
    if (!table || !key || !value)
        return false;

    unsigned long hash = hash_murmur3_32(key, table->key_size) % table->num_of_buckets;

    node_t *current = table->buckets[hash];
    while (current)
    {
        if (!current->is_free && !memcmp(current->key, key, table->key_size))
        {
            memcpy(value, current->value, table->value_size);
            return true;
        }
        current = current->next;
    }

    return false;
}