#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../hashmap.h"

// simple hash function for strings
inline size_t string_hash(const void *key) {
    const char *str = key;
    size_t hash = 0;
    while (*str) {
        hash = hash * 31 + *str++;
    }
    return hash;
}


void test_hashmap() {
    printf("🚀 Starting hashmap tests...\n");

    // Initialize hashmap
    hashmap *map = hashmap_init(5, sizeof(char[50]), sizeof(int), NULL, NULL);
    assert(map != NULL);
    assert(hashmap_is_empty(map));

    // insert key-value pairs
    int val1 = 42, val2 = 99, val3 = 123;
    hashmap_put(map, "apple", &val1);
    hashmap_put(map, "banana", &val2);
    hashmap_put(map, "cherry", &val3);

    assert(hashmap_is_empty(map) == false);
    
    // check retrieval
    int result;
    assert(hashmap_get(map, "apple", &result) && result == 42);
    assert(hashmap_get(map, "banana", &result) && result == 99);
    assert(hashmap_get(map, "cherry", &result) && result == 123);
    
    // check existence
    assert(hashmap_contains(map, "apple"));
    assert(hashmap_contains(map, "banana"));
    assert(hashmap_contains(map, "grape") == false);

    // update existing key
    int new_val = 555;
    hashmap_put(map, "banana", &new_val);
    assert(hashmap_get(map, "banana", &result) && result == 555);

    // remove a key
    hashmap_remove(map, "banana");
    assert(hashmap_contains(map, "banana") == false);
    assert(hashmap_get(map, "banana", &result) == false);

    // try removing a non-existent key
    hashmap_remove(map, "banana");  // Should not crash

    // insert many elements to test collisions
    for (int i = 0; i < 20; i++){
        char key[10];
        sprintf(key, "key%d", i);
        hashmap_put(map, key, &i);
    }

    // check that they are all present
    for (int i = 0; i < 20; i++){
        char key[10];
        sprintf(key, "key%d", i);
        assert(hashmap_get(map, key, &result) && result == i);
    }

    hashmap_destroy(map);

    printf("\033[32mOK\033[0m All tests passed!\n");
}

int main() {
    test_hashmap();
    return 0;
}
