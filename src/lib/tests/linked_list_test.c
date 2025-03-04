#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "../linked_list.h"

// comparison function for ints
bool compare_ints(void *data1, void *data2){
    return *(int*)data1 < *(int*)data2;
}

// print function for ints
void print_int(void *data){
    printf("%d ", *(int*)data);
}

int main(){
    printf("🚀 Starting linked list tests...\n");

    list_head *list = list_init(sizeof(int));

    // test list_insert_front
    int val = 10;
    list_insert_front(list, &val);
    printf("After inserting 10 at front: ");
    list_print(list, print_int);
    printf("\n");
    
    // test list_peek_front
    int peek_front;
    list_peek_front(list, &peek_front);
    printf("Peek front: %d\n", peek_front);
    
    // test list_peek_back
    int peek_back;
    list_peek_back(list, &peek_back);
    printf("Peek back: %d\n", peek_back);
    
    // test list_remove_front
    int removed_front;
    list_remove_front(list, &removed_front);
    printf("Removed front: %d\n", removed_front);

    // test list_remove_back
    int val2 = 10;
    list_insert_front(list, &val2);
    int removed_back;
    list_remove_back(list, &removed_back);
    printf("Removed back: %d\n", removed_back);

    // test list_remove_node
    int val3 = 10;
    list_insert_front(list, &val3);
    int removed_node;
    list_remove_node(list, 0, &removed_node);
    printf("Removed node: %d\n", removed_node);

    int data[] = {5, 3, 1, 4, 13, 6, 10, 7, 8, 9, 0, 11, 14, 12, 15, 2};
    for(size_t i=0; i<sizeof(data)/sizeof(int); i++){
        list_insert_back(list, &data[i]);
    }

    // print before sorting
    printf("Before sorting: ");
    list_print(list, print_int);
    printf("\n");
    // test merge sort
    list_merge_sort(list, compare_ints);

    // print after sorting
    printf(" After sorting: ");
    list_print(list, print_int);
    printf("\n");
    
    // test remove_node with more than one element
    int removed_node2;
    list_remove_node(list, 5, &removed_node2);
    printf("\nShould remove node Nr 5. List should look like this: \n");
    printf("0 1 2 3 4 6 7 8 9 10 11 12 13 14 15\n\n");
    printf("Actually removed node Nr %d. List actually looks like this: \n", removed_node2);
    list_print(list,print_int);
    printf("\n\n");


    // test remove_node with index out of bounds
    printf("Now attempting to remove node with index out of bounds (this should fail): \n");
    list_remove_node(list, 100, &removed_node2);

    // cleanup
    list_destroy(list);
    return 0;
}