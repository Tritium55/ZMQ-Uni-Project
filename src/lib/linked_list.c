#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct list_node list_node;

// debug mode (set to one to enable debug print)
#define DEBUG_PRINT 0

// linked list functions
list_head* list_init(size_t data_size){
    assert(data_size);
    list_head *new_list = (list_head *) calloc(1, sizeof(list_head));
    if(!new_list){
        fprintf(stderr, "Could not allocate new list header.\n");
        exit(1);
    }
    new_list->first = NULL;
    new_list->last = NULL;
    new_list->data_size = data_size;
    return new_list;
}

bool list_is_empty(list_head *head){
    assert(head);
    if(head->first == NULL && head->last == NULL)
        return true;
    
    else if(head->first != NULL && head->last != NULL)
        return false;
    
    else{
        // this should never happen
        fprintf(stderr, "Linked List pointer error.\nFirst pointer: %p\nLast Pointer: %p\n",
                (void *) head->first, (void *) head->last);
        exit(1);
    }
}

void list_insert_front(list_head *head, const void *data){
    assert(head);
    assert(data);

    // allocates the user specified data size + the size of the list_node struct
    list_node *new_elem = (list_node *) calloc(1, sizeof(list_node) + head->data_size);
    if(!new_elem){
        fprintf(stderr, "Could not allocate new list node.\n");
        exit(1);
    }

    // copy the data into the new node
    memcpy(new_elem->data, data, head->data_size);

    new_elem->prev = NULL;
    new_elem->next = head->first;
    head->first = new_elem;

    // edge case if the list now has one element
    if(head->last == NULL)
        head->last = new_elem;
    else
        head->first->next->prev = new_elem;     // more than one element is in the list

    return;
}

void list_insert_back(list_head *head, const void *data){
    assert(head);
    assert(data);

    // allocates the user specified data + size of the list_node struct
    list_node *new_elem = (list_node *) calloc(1, sizeof(list_node) + head->data_size);
    if(!new_elem){
        fprintf(stderr, "Could not allocate new list node.\n");
        exit(1);
    }

    // copy the data into the new node
    memcpy(new_elem->data, data, head->data_size);

    new_elem->prev = head->last;
    new_elem->next = NULL;
    head->last = new_elem;

    // edge case if the list now has one element
    if(head->first == NULL)
        head->first = new_elem;
    else
        head->last->prev->next = new_elem;     // more than one element is in the list

    return;
}

void list_peek_front(list_head *head, void *data){
    assert(head);
    assert(data);
    if(list_is_empty(head)){
        fprintf(stderr, "List is empty on attempt to peek.\n");
        return;
    }

    memcpy(data, head->first->data, head->data_size);
    return;
}

void list_peek_back(list_head *head, void *data){
    assert(head);
    assert(data);
    if(list_is_empty(head)){
        fprintf(stderr, "List is empty on attempt to peek.\n");
        return;
    }

    memcpy(data, head->last->data, head->data_size);
    return;
}

void list_remove_front(list_head *head, void *data){
    assert(head);
    if(list_is_empty(head)){
        fprintf(stderr, "List is empty on attempt to remove from front.\n");
        return;
    }

    list_node *temp = head->first;
    // copy data into buffer if data is not NULL
    // this allows the user to dump the data and just remove the node by passing NULL
    if(data)
        memcpy(data, temp->data, head->data_size);
    
    head->first = temp->next;

    // edge case if the list is empty after removing the last element
    if(head->first == NULL)
        head->last = NULL;
    else
        head->first->prev = NULL;       // more than one element is left

    free(temp);
    return;
}

void list_remove_back(list_head *head, void *data){
    assert(head);
    if(list_is_empty(head)){
        fprintf(stderr, "List is empty on attempt to remove from back.\n");
        return;
    }

    list_node *temp = head->last;
    // copy data into buffer if data is not NULL
    // this allows the user to dump the data and just remove the node by passing NULL
    if(data)
        memcpy(data, temp->data, head->data_size);
    
    head->last = temp->prev;

    // edge case if the list is empty after removing the last element
    if(head->last == NULL)
        head->first = NULL;
    else
        head->last->next = NULL;        // more than one element is left

    free(temp);
    return;
}

void list_remove_node(list_head *head, size_t index, void *data){
    assert(head);
    if(list_is_empty(head)){
        fprintf(stderr, "List is empty on attempt to remove node by index.\n");
        return;
    }

    list_node *temp = head->first;
    for(size_t i=0; i<index; i++){
        if(temp->next)
            temp = temp->next;
        else{
            fprintf(stderr, "Index out of bounds on attempt to remove node by index.\n");
            return;
        }
    }

    // copy data into buffer if data is not NULL
    // this allows the user to dump the data and just remove the node by passing NULL
    if(data)
        memcpy(data, temp->data, head->data_size);

    // pointer bending bs
    if(temp->prev == NULL && temp->next == NULL){   // only one element in list
        head->first = NULL;
        head->last = NULL;
    }
    else if(temp->prev == NULL){                    // first element
        head->first = temp->next;
        temp->next->prev = NULL;
    }
    else if(temp->next == NULL){                    // last element
        head->last = temp->prev;
        temp->prev->next = NULL;
    }
    else{                                           // somewhere in the middle
        temp->prev->next = temp->next;
        temp->next->prev = temp->prev;
    }

    free(temp);
    return;
}

void list_print(list_head *head, void (*print_function)(void *data)){
    if(!head || list_is_empty(head)){
        printf("List is empty on attempt to print.\n");
        return;
    }

    assert(print_function);

    if(DEBUG_PRINT)
        printf("\nList: \n\n");

    list_node *temp = head->first;
    while(temp){
        if(DEBUG_PRINT){
            if(temp->prev)
                printf(" %p <--  ", (void *)temp->prev);
            else
                printf(" |-----------------  ");
        }
        
        print_function(temp->data);

        if(DEBUG_PRINT){
            if(temp->next)
                printf("  --> %p \n", (void *)temp->next);
            else
                printf("  -----------------|\n");
        }
        
        temp = temp->next;
    }

    if(DEBUG_PRINT){
        if(head->first->prev == NULL && head->last->next == NULL)
            printf("\n                     \033[32mOK\033[0m\n");
    }

    return;
}

void list_destroy(list_head *head){
    assert(head);
    list_node *temp = NULL;
    while(head->first){
        temp = head->first->next;
        free(head->first);
        head->first = temp;
    }
    free(head);
    return;
}

void list_nested_destroy(list_head *head, void (*destroy_function)(void *data)){
    assert(head);
    assert(destroy_function);
    list_node *temp = NULL;
    while(head->first){
        temp = head->first->next;
        destroy_function(head->first->data);
        free(head->first);
        head->first = temp;
    }
    free(head);
    return;
}


// merge sort function section:
// split the list into two halves
static void split_list(list_node *source, list_node **front, list_node **back){
    list_node *slow = source;
    list_node *fast = source->next;

    // the fast pointer moves twice as fast as the slow pointer, thus splitting the list in half
    while(fast){
        fast = fast->next;
        if(fast){
            slow = slow->next;
            fast = fast->next;
        }
    }

    *front = source;
    *back = slow->next;
    slow->next = NULL;
    return;
}

// merge sort helper
static list_node *merge_sorted_lists(list_node *a, list_node *b, bool (*compare_function)(void *data1, void *data2)){
    if(!a)
        return b;
    if(!b)
        return a;

    list_node *result = NULL;
    if(compare_function(a->data, b->data)){
        result = a;
        result->next = merge_sorted_lists(a->next, b, compare_function);
    }
    else{
        result = b;
        result->next = merge_sorted_lists(a, b->next, compare_function);
    }

    if(result->next)
        result->next->prev = result;
        
    result->prev = NULL;
    return result;
}

// merge sort function itself
void merge_sort(list_node **head, bool (*compare_function)(void *data1, void *data2)){
    list_node *temp = *head;
    if(temp == NULL || temp->next == NULL)
        return;
    
    list_node *a = NULL;
    list_node *b = NULL;

    split_list(temp, &a, &b);

    merge_sort(&a, compare_function);
    merge_sort(&b, compare_function);

    *head = merge_sorted_lists(a, b, compare_function);
    return;
}

// wrapper function
void list_merge_sort(list_head *head, bool (*compare_function)(void *data1, void *data2)){
    assert(head);
    assert(compare_function);
    merge_sort(&head->first, compare_function);

    // update last pointer
    list_node *temp = head->first;
    while(temp && temp->next)
        temp = temp->next;
    
    head->last = temp;
    return;
}