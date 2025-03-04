#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "./hashmap.h"

// set to 1 for debug output
#define DEBUG_PRINT 0

typedef struct{
    char *key;          // raw byte array nr 1
    char *value;        // raw byte array nr 2 (yes, I made this comment because I think it's funny)
}hashmap_entry;

// djb2 hash function
// assuming the key is a string
// yeah, I saw a thread on reddit...
static inline size_t default_hash_function(const void *key){
    const char *str = (const char *) key;
    size_t hash = 5381;
    
    int character = 0;
    while((character = *str++)){
        hash = ((hash << 5) + hash) + character;    // hash * 33 + character
    }

    return hash;
}

// assuming the key is a string
static inline bool default_compare_function(const void *key1, const void *key2){
    bool result = strcmp((const char *) key1, (const char *) key2);
    return (result == 0);
}

// still assuming that the key is a string and that the value is an int
static void default_print_entry(void *data){
    hashmap_entry *entry = (hashmap_entry *) data;
    printf("key: %s, value: %d\n", entry->key, *(int *)entry->value);
}

// hashmap functions
hashmap* hashmap_init(size_t list_amount, size_t key_size, size_t value_size,
                      size_t (*hash_function)(const void *key),
                      bool (*compare_function)(const void *key1, const void *key2)){
    
    hashmap *new_map = (hashmap *) calloc(1, sizeof(hashmap));
    if(!new_map){
        fprintf(stderr, "Could not allocate new hashmap.\n");
        exit(1);
    }

    new_map->list_amount = list_amount;
    new_map->key_size = key_size;
    new_map->value_size = value_size;

    if(hash_function)
        new_map->hash_function = (size_t (*)(const void*))hash_function;
    else
        new_map->hash_function = (size_t (*)(const void*))default_hash_function;

    if(compare_function)
        new_map->compare_function = (bool (*)(const void*, const void*))compare_function;
    else
        new_map->compare_function = (bool (*)(const void*, const void*))default_compare_function;

    // allocate memory for all lists
    new_map->lists = (list_head **) calloc(list_amount, sizeof(list_head *));
    if(!new_map->lists){
        fprintf(stderr, "Could not allocate memory for hashmap lists.\n");
        free(new_map);
        exit(1);
    }

    for(size_t i=0; i<list_amount; i++){
        new_map->lists[i] = list_init(sizeof(hashmap_entry));
    }

    return new_map;
}

bool hashmap_is_empty(hashmap *map) {
    assert(map);
    for(size_t i=0; i<map->list_amount; i++){
        if(!list_is_empty(map->lists[i]))
            return false;
    }
    return true;
}

bool hashmap_contains(hashmap *map, const void *key){
    assert(map);
    assert(key);
    if(hashmap_is_empty(map))
        return false;

    size_t index = map->hash_function(key) % map->list_amount;
    list_head *list = map->lists[index];

    struct list_node *curr = list->first;
    while(curr){
        hashmap_entry *entry = (hashmap_entry *) curr->data;
        if(map->compare_function(key, entry->key)){
            return true;
        }
        curr = curr->next;
    }

    return false;
}

void hashmap_put(hashmap *map, const void *key, const void *value){
    assert(map);
    assert(key);
    assert(value);

    size_t index = map->hash_function(key) % map->list_amount;
    list_head *list = map->lists[index];

    struct list_node *curr = list->first;
    while(curr){
        hashmap_entry *entry = (hashmap_entry *) curr->data;
        if(map->compare_function(key, entry->key)){
            memcpy(entry->value, value, map->value_size);
            return;
        }
        curr = curr->next;
    }

    // this can be static, as it is just being copied into the list (and allocated there), thus existing beyond this scope
    hashmap_entry new_entry;
    new_entry.key = (char *) calloc(1, map->key_size);
    new_entry.value = (char *) calloc(1, map->value_size);
    if(!new_entry.key || !new_entry.value){
        fprintf(stderr, "Could not allocate new hashmap entry key or value.\n");
        exit(1);
    }

    memcpy(new_entry.key, key, map->key_size);
    memcpy(new_entry.value, value, map->value_size);

    list_insert_back(list, &new_entry);
    return;
}

bool hashmap_get(hashmap *map, const void *key, void *value){
    assert(map);
    assert(key);
    assert(value);

    size_t index = map->hash_function(key) % map->list_amount;
    list_head *list = map->lists[index];

    struct list_node *curr = list->first;
    while(curr){
        hashmap_entry *entry = (hashmap_entry *) curr->data;
        if(map->compare_function(key, entry->key)){
            memcpy(value, entry->value, map->value_size);
            return true;
        }
        curr = curr->next;
    }

    return false;
}

void hashmap_remove(hashmap *map, const void *key){
    assert(map);
    assert(key);

    size_t index = map->hash_function(key) % map->list_amount;
    list_head *list = map->lists[index];

    struct list_node *curr = list->first;
    size_t pos = 0;
    while(curr){
        hashmap_entry *entry = (hashmap_entry *) curr->data;
        if(map->compare_function(entry->key, key)){
            free(entry->key);
            free(entry->value);
            list_remove_node(list, pos, NULL);
            return;
        }
        curr = curr->next;
        pos++;
    }
    return;
}

// print_function should print the data of one single hashmap entry
void hashmap_print(hashmap *map, void (*print_function)(void *data)){
    assert(map);
  
    void (*func)(void *);
    if(print_function){
        func = print_function;
    }
    else{
        func = default_print_entry;
    }
        

    if(DEBUG_PRINT)
        printf("\n\n\nHashmap:\n\n");
    for(size_t i=0; i<map->list_amount; i++){
        if(DEBUG_PRINT)
            printf("Hashmap list %zu:\n", i);
        
        if(!list_is_empty(map->lists[i]))
            list_print(map->lists[i], (void (*)(void*))(func));
        
        if(DEBUG_PRINT)
            printf("\n");

    }
}


void hashmap_to_string(hashmap *map, char *buffer, void (*to_string_function)(char *buffer, void *key, void *value)){
    assert(map);
    assert(buffer);
    assert(to_string_function);

    buffer[0] = '\0';
    size_t current_start_of_word = 0;

    for(size_t i=0; i<map->list_amount; i++){
        list_head *list = map->lists[i];
        
        struct list_node *curr = list->first;
        while(curr){
            hashmap_entry *entry = (hashmap_entry *) curr->data;
            to_string_function(&buffer[current_start_of_word], entry->key, entry->value);
            current_start_of_word = strlen(buffer);
            curr = curr->next;
        }
    }

    return;
}

// helper function
static inline void destroy_entry(void *data){
    hashmap_entry *entry = (hashmap_entry *) data;
    free(entry->key);
    free(entry->value);
    return;
}

void hashmap_remove_all_elements(hashmap *map, void (*handle_each_element)(void *key, void *value)){
    assert(map);
    assert(handle_each_element);

    for(size_t i=0; i<map->list_amount; i++){
        while(!list_is_empty(map->lists[i])){
            hashmap_entry entry;
            entry.key = NULL;
            entry.value = NULL;
            list_remove_front(map->lists[i], &entry);
            handle_each_element(entry.key, entry.value);
            destroy_entry(&entry);
        }
    }
}

void hashmap_destroy(hashmap *map){
    assert(map);
    for(size_t i=0; i<map->list_amount; i++){
        list_nested_destroy(map->lists[i], destroy_entry);
    }

    free(map->lists);
    free(map);
    return;
}
