#pragma once

// This header houses a hashmap
// it uses a linked list instead of linear probing
// it is dependent on the linked_list.h header from this project and the way it's data is implemented
// todo: use self balancing trees for the buckets or double hashing (more efficient and optimized and fancy)
#include <stddef.h>
#include <stdbool.h>
#include "./linked_list.h"


typedef struct{
    list_head **lists;      // array of linked lists for chaining
    size_t list_amount;     // amount of lists in array
    size_t key_size;        // data size of key
    size_t value_size;      // data size of the value stored
    size_t (*hash_function)(const void *key);       // optional (default is djb2), but recommended if you want something else
    bool (*compare_function)(const void *key1, const void *key2);   // optional (default is for strings as key), but should return true if keys are the same
}hashmap;

// the compare_function needs to return true if the keys are equal
// I recommend using inline functions for both the hash and compare functions
hashmap* hashmap_init(size_t list_amount, size_t key_size, size_t value_size,
                      size_t (*hash_function)(const void *key),
                      bool (*compare_function)(const void *key1, const void *key2));
bool hashmap_is_empty(hashmap *map);
bool hashmap_contains(hashmap *map, const void *key);
// if the key already exists, the value will be overwritten
void hashmap_put(hashmap *map, const void *key, const void *value);
// you can supply a buffer for the data to be copied into (this can be static :])
bool hashmap_get(hashmap *map, const void *key, void *value);
void hashmap_remove(hashmap *map, const void *key);
void hashmap_print(hashmap *map, void (*print_function)(void *data));
// expects a buffer of sufficient size for storing the result, writing itself is happening in the to_string_function
void hashmap_to_string(hashmap *map, char *buffer, void (*to_string_function)(char *buffer, void *key, void *value));
// it empties the hashmap, but it doesn't free it
// isn't really random, just picks the first best thing to remove (and again and again and again)
// this is useful if you want to convert the hashmap datastructure into something else
void hashmap_remove_all_elements(hashmap *map, void (*handle_each_element)(void *key, void *value));
void hashmap_destroy(hashmap *map);
// todo: hashmap_destroy_nested(hashmap *map, void (*destroy_function)(void *key, void *value));
