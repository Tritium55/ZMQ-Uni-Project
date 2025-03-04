#pragma once

// This header houses a doubly linked list and merge sort

#include <stddef.h>
#include <stdbool.h>

// structs for linked list
struct list_node{
    struct list_node *next;
    struct list_node *prev;
    char data[];                        // flexible array member
};

typedef struct{
    struct list_node *first;
    struct list_node *last;
    size_t data_size;
}list_head;

// linked list functions
list_head* list_init(size_t data_size);
bool list_is_empty(list_head *head);
// all of the following functions expect the address of e.g. a struct (yes, it can be static :] , that's why we do all of this) as <data>
void list_insert_front(list_head *head, const void *data);
void list_insert_back(list_head *head, const void *data);
void list_peek_front(list_head *head, void *data);
void list_peek_back(list_head *head, void *data);
// data may also be NULL here (just on the remove funcs), if you don't care about the data and just want to remove the node
void list_remove_front(list_head *head, void *data);
void list_remove_back(list_head *head, void *data);
void list_remove_node(list_head *head, size_t index, void *data);   // index starts at 0

// print_function needs to print a data segment of one single node
void list_print(list_head *head, void (*print_function)(void *data));

// regular destroy function
void list_destroy(list_head *head);
// this destroy function expects a function that destroys the data segment of one single node
// this is useful if you have a list of structs that contain pointers to dynamically allocated memory
void list_nested_destroy(list_head *head, void (*destroy_function)(void *data));



// merge sort function
// compare_function needs to supply a function that compares two data segments and returns true if the first one is smaller than the second one
void list_merge_sort(list_head *head, bool (*compare_function)(void *data1, void *data2));

// linked list example for integers:
/*
// data struct you want to save in the list
typedef struct{
    char some_data[100];
    int more_data;
}my_data;

void my_print_function(void *data){
    my_data *temp = (my_data*) data;
    printf("%s\n", temp->some_data);
    printf("%d\n", temp->more_data);
}

int main(){
    // remember: if you want to store a string, reserve enough room for the '\0' (if you want your code to be POSIX conform)
    list_head* list = list_init(sizeof(my_data));
    my_data new_data;
    strcpy(new_data.some_data, "Hello World!\n");           // save some stuff in the struct
    new_data.more_data = 500;

    * add data to list
    list_insert_front(list, &new_data);         // this passes the struct by reference and it is getting allocated by the linked list library
    
    * print the list
    list_print(list, my_print_function);

    * remove data from list
    my_data old_data;
    list_remove_front(list, &old_data);         // this saves the data from the removed node to "old_data"
    
    ! all of this seems ugly, but it makes cleanup a lot easier

    * destroying a list
    list_destroy(list);
}
*/