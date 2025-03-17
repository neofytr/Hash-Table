#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct node
{
    void *key;
    void *value;
    bool is_free;
    struct node *next;
} node_t;

typedef bool (*hash_value_add)(const void *val_one, const void *val_two, const void *result);

typedef struct
{
    size_t num_of_buckets; // number of buckets you want in the hashtable
    size_t key_size;
    size_t value_size;
    node_t **buckets;   // each bucket is a linked list of nodes
    node_t *free_nodes; // list of free nodes that can be reused
    size_t num_of_nodes;

} hash_table_t;

hash_table_t *hash_table_create(size_t num_of_buckets, size_t key_size, size_t value_size); // number of buckets you want in the hashtable
                                                                                            // each bucket is a linked list of nodes
void hash_table_destroy(hash_table_t *table);

bool hash_table_insert(hash_table_t *table, const void *key, const void *value);
bool hash_table_delete(hash_table_t *table, const void *key);
bool hash_table_search(hash_table_t *table, const void *key, void *value);
bool hash_table_clear(hash_table_t *table);
hash_table_t *hash_table_merge(hash_table_t **hash_table_arr, size_t len, hash_value_add add_value, size_t key_size, size_t value_size, size_t new_bucket_num);

#endif