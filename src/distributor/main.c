// This is the server side of this project

#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include "../lib/encoder.h"
#include "../lib/linked_list.h"
#include "../lib/hashmap.h"

#define MSG_LEN 1500


static inline void print_int(void *data){
    int temp = *(int *)data;
    printf("%d ", temp);
}

// copy pasted this from the std c lib (would be easy to implement tho)
static inline bool is_alpha(char c){
    return (c>='a' && c<='z') || (c>='A' && c<='Z');
}

typedef struct{
    char word[MSG_LEN];     //? this isn't memory efficient, but it can't fail any test, where a word is stupidly long
    int amount;
}key_value_pair;

// compare function for list_merge_sort function
bool compare_pairs(void *data1, void *data2){
    key_value_pair one = *(key_value_pair *) data1;
    key_value_pair two = *(key_value_pair *) data2;
    if(one.amount < two.amount)
        return true;
    if(one.amount > two.amount)
        return false;
    
    if(one.amount == two.amount){
        int temp = strcmp(one.word, two.word);
        if(temp>0)
            return true;
        if(temp<0)
            return false;
        
        // this should never trigger
        fprintf(stderr, "Two words within the linked list match after combining every key,value pair");
        exit(1);
    }

    // just because of ICO C standards (-Wreturn-type)
    // in reality, this will never be reached
    return false;
}

// this is very ugly but it kinda works
static list_head *result;

// helper func for hashmap_to_linked_list
void handle_each_hashmap_element(void *key, void *value){
    key_value_pair pair;
    strcpy(pair.word, (char*)key);
    pair.amount = *(int*)value;

    assert(result);
    list_insert_back(result, &pair);
}

// wrapper func
void hashmap_to_linked_list(hashmap *map){
    result = list_init(sizeof(key_value_pair));
    hashmap_remove_all_elements(map, handle_each_hashmap_element);
}

// load balancing function
// copies the file in chunks [type: char*] to the supplied linked list
// returns -1 on failure, 0 on success
// this func assumes the file is open with read privileges
int assign_next_file_chunk_to_list(list_head *list, FILE *fp, unsigned long long file_size){
    if(!list || !fp)
        return -1;
    
    if(file_size == 0){
        char temp[MSG_LEN-3] = {0};
        list_insert_back(list, temp);
        return 0;
    }

    unsigned long long current_offset = 0;       // keeps track of last assigned chunk

    jumpback_label: ;       // this isn't pretty, but it is in case there is a chunk of chunk_size (MSG_LEN-4 bytes) inside the file, where no word can be found

    while(true){
        char chunk[MSG_LEN-3] = {0};
        // no more chunks left
        if(current_offset >= file_size)
            return 0;
    
        // MSG_LEN-4 because the first 3 bytes are reserved for command and one is needed for NUL string terminator
        size_t chunk_size = MSG_LEN - 4;

        // check if this is the last chunk
        if(current_offset + chunk_size >= file_size){
            chunk_size = file_size - current_offset;
        }

        // assert no word splitting
        // preventing possibility of it splitting the chunk in a wrong way. a sequence inside the file, which could look like this:
        // "word1111another11" must be kept intact and cannot be split into "word111" and "1another11"
        size_t temp = chunk_size;
        if(current_offset + chunk_size < file_size){            // rest of file is too big for chunk
            fseek(fp, temp-1, SEEK_CUR);                        // move to end of chunk
            char c = fgetc(fp);
            while(c != EOF && !is_alpha(c)){                     // landed on a non char -> need to move to previous word
                temp--;
                if(temp == 0){  // cannot find a word in chunk
                    fprintf(stderr, "Could not find word in chunk. Skipping chunk.");
                    current_offset += chunk_size;
                    goto jumpback_label;
                }

                fseek(fp, -2, SEEK_CUR);            // -2 because fgetc moved one forward so we have to compensate
                c = fgetc(fp);
            }

            while(c != EOF && is_alpha(c)){                     // now on a char (landed or moved back) -> need to move to next non word
                temp--;
                if(temp == 0)   // cannot find the ending of a word in chunk -> no valid chunk selection possible
                    return 0;

                fseek(fp, -2, SEEK_CUR);            // -2 because fgetc moved one forward so we have to compensate
                c = fgetc(fp);
            }
        }

        // copy chunk to buffer
        fseek(fp, current_offset, SEEK_SET);    // move to start of chunk
        if(fread(chunk, 1, temp, fp) != temp){
            return 0;
        }

        if(temp == MSG_LEN - 4)                 // chunk len is maximum
            chunk[MSG_LEN-3] = '\0';
        else
            chunk[temp] = '\0';

        current_offset += temp;
        list_insert_back(list, chunk);
    }
}

void print_result_to_stdout(){
    printf("word,frequency\n");
    while(!list_is_empty(result)){
        key_value_pair pair;
        list_remove_back(result, &pair);
        if(pair.word[0] == '\0')
            continue;
        printf("%s,%d\n", pair.word, pair.amount);
    }
}


typedef struct{
    void *context;
    int port;
    MSG_TYPE command;
    char chunk[MSG_LEN];
}worker_data;

void print_job(void *data){
    char *temp = (char *)data;
    printf("%s\n\n",temp);
}

void *handle_worker_thread(void *data){
    assert(data);
    worker_data *worker = (worker_data *) data;
    assert(worker->context);
    assert(worker->port > 0);

    void *distributor = zmq_socket(worker->context, ZMQ_REQ);
    if(!distributor){
        fprintf(stderr, "Could not create ZMQ socker: %s\n", zmq_strerror(zmq_errno()));
        pthread_exit(NULL);
    }

    // convert port number to string and copy to buffer
    char port_buff[30] = {0};
    if(snprintf(port_buff, sizeof(port_buff), "tcp://localhost:%d", worker->port) < 0){
        fprintf(stderr, "Port number could not be copied.\n");
        pthread_exit(NULL);
    }

    // connect to port
    int rc = zmq_connect(distributor, port_buff);
    if(rc != 0){
        fprintf(stderr, "Could not connect to port %d (ZMQ error): %s\n", worker->port, zmq_strerror(zmq_errno()));
        fprintf(stderr, "Command: %d\n", worker->command);
        zmq_close(distributor);
        pthread_exit(NULL);
    }

    // handle worker
    char buffer[MSG_LEN] = {0};
    if(encode_msg_to_worker(buffer, worker->chunk, worker->command) != 0){
        fprintf(stderr, "Could not encode message.\n\n");
        pthread_exit(NULL);
    }

    zmq_send(distributor, buffer, strlen(buffer)+1, 0);

    memset(buffer, 0, sizeof(buffer));
    memset(worker->chunk, 0, sizeof(worker->chunk));
    zmq_recv(distributor, buffer, MSG_LEN, 0);
    worker->command = decode_msg_from_worker(buffer, worker->chunk);

    zmq_close(distributor);
    return worker;
    pthread_exit(NULL);
}


int main(int argc, char **argv){
    unsigned int amount_of_ports = 0;
    if(argc<=2){
        fprintf(stderr, "Not enough arguments: %d", argc);
        exit(1);
    }
    
    amount_of_ports = (unsigned int) argc - 2;     // subtract 2 because program name and file name are the first two parameters
    assert(amount_of_ports>0);

    // parse all port numbers
    int ports[(const unsigned int)amount_of_ports];
    for(unsigned int i=0; i<amount_of_ports; i++){
        ports[i] = atoi(argv[i+2]);
    }

    // open file
    FILE *fp;
    fp = fopen(argv[1], "r");
    if(fp == NULL){
        fprintf(stderr, "Could not open file\n");
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    unsigned long long file_size = ftell(fp);   // size of file in bytes
    rewind(fp);
   
    // worker handling bs begins here
    void *context = zmq_ctx_new();
    pthread_t worker_handlers[(const unsigned int)amount_of_ports];

    // I am using two linked list to queue tasks (one for the jobs themselves and one for the workers, that aren't busy atm)
    // I also need one to check which tasks we need to join next
    list_head *workers_waiting_queue = list_init(sizeof(int));
    list_head *workers_running_queue = list_init(sizeof(int));
    list_head *task_queue = list_init(sizeof(char) * (MSG_LEN-3));


    // MAP SECTION
    // queue all MAP tasks
    if(assign_next_file_chunk_to_list(task_queue, fp, file_size)){
        fprintf(stderr, "Assigning tasks went wrong.\n");
        exit(1);
    }
    fclose(fp); // file no longer needed

    // todo: remove print statements
    //printf("\n\n\nMAP JOBS:\n");
    //list_print(task_queue, print_job);

    for(unsigned int i=0; i<amount_of_ports; i++){
        list_insert_back(workers_waiting_queue, &i);            // saves the index of the worker in the queue -> pthread_t worker_handlers and int ports and be accessed this way
    }
    
    // temp file for result of map
    FILE *map_temp_file = fopen("map_results.txt", "w+");
    if(map_temp_file == NULL){
        fprintf(stderr, "Failed to create temporary file for MAP results with write privileges.\n");
        exit(1);
    }

    // running MAP
    while(!list_is_empty(task_queue) || !list_is_empty(workers_running_queue)){
        if(!list_is_empty(workers_waiting_queue) && !list_is_empty(task_queue)){
            int worker_nr = -1;
            list_remove_front(workers_waiting_queue, &worker_nr);

            worker_data *temp = (worker_data *) calloc(1, sizeof(worker_data));
            if(!temp){
                fprintf(stderr, "Could not allocate worker data as thread argument.\n");
                exit(1);
            }

            assert(worker_nr != -1);
            
            list_remove_front(task_queue, temp->chunk);
            temp->port = ports[worker_nr];
            temp->command = MAP;
            temp->context = context;
            pthread_create((pthread_t *)&worker_handlers[worker_nr], NULL, handle_worker_thread, (void *)temp);
            list_insert_back(workers_running_queue, &worker_nr);
        }
        else{       // this could be optimized by using a thread pool instead of creating and destroying threads
            int worker_index = -1;
            list_remove_front(workers_running_queue, &worker_index);
            assert(worker_index != -1);

            worker_data *temp = NULL;
            int res = pthread_join((pthread_t)worker_handlers[worker_index], (void **)&temp);
                
            assert(temp);
            assert(res == 0);

            fprintf(map_temp_file, "%s", temp->chunk);        // save map output to map_temp_file
            list_insert_back(workers_waiting_queue, &worker_index);
            free(temp);
        }
    }
    assert(list_is_empty(workers_running_queue));
    fclose(map_temp_file);


    // RED SECTION
    map_temp_file = fopen("map_results.txt", "r");
    if (!map_temp_file) {
        fprintf(stderr, "Failed to open temporary file of MAP results with read privileges.\n");
        exit(1);
    }

    fseek(map_temp_file, 0L, SEEK_END);
    file_size = ftell(map_temp_file);   // size of file in bytes
    rewind(map_temp_file);

    if(assign_next_file_chunk_to_list(task_queue, map_temp_file, file_size)){
        fprintf(stderr, "Assigning tasks went wrong\n.");
        exit(1);
    }
    fclose(map_temp_file);
    // todo: uncomment remove call
    remove("map_results.txt");      // cleanup

    // todo: remove print calls
    //printf("\n\n\nRED JOBS:\n");
    //list_print(task_queue, print_job);

    hashmap *map = hashmap_init(50, sizeof(char) * MSG_LEN, sizeof(int), NULL, NULL);
    while(!list_is_empty(task_queue) || !list_is_empty(workers_running_queue)){
        if(!list_is_empty(workers_waiting_queue) && !list_is_empty(task_queue)){
            int worker_nr = -1;
            list_remove_front(workers_waiting_queue, &worker_nr);
            worker_data *temp = (worker_data *) calloc(1, sizeof(worker_data));
            if(!temp){
                printf("Could not allocate worker data as thread argument.\n");
                exit(1);
            }
            assert(worker_nr != -1);

            list_remove_front(task_queue, (void *)temp->chunk);
            temp->port = ports[worker_nr];
            temp->command = RED;
            temp->context = context;
            pthread_create((pthread_t *)&worker_handlers[worker_nr], NULL, handle_worker_thread, (void *)temp);
            list_insert_back(workers_running_queue, &worker_nr);
        }
        // list is empty
        else{       // this could be optimized by sleeping and getting an interrupt on worker thread exit
            int worker_index = -1;
            list_remove_front(workers_running_queue, &worker_index);
            assert(worker_index != -1);

            worker_data *temp = NULL;
            int res = pthread_join((pthread_t)worker_handlers[worker_index], (void **)&temp);

            if(!temp || res != 0){
                fprintf(stderr, "Joined Thread %d running on Port %d returned NULL instead of valid worker_data pointer.\n", worker_index, ports[worker_index]);
                fprintf(stderr, "pthread_join returned %d\n", res);
                exit(1);
            }

            // add data from workers to hashmap
            key_value_pair pair;
            pair.amount = 0;
            pair.word[0] = '\0';
            int word_end = 0;
            int value_end = 0;
            char value_as_string[10] = {0};  // 10 digits should be enough (if not, the one who made the testbench is smoking some good stuff)
            for(int j=0; temp->chunk[j] != '\0'; j++){
                if(is_alpha(temp->chunk[j])){
                    if(value_end > 0){
                        // new word begins -> add last word and number to hashmap
                        value_as_string[value_end] = '\0';
                        value_end = 0;

                        pair.amount = atoi(value_as_string);
                        int word_count = pair.amount;
                        if(hashmap_contains(map, pair.word)){
                            hashmap_get(map, pair.word, &word_count);
                            word_count += pair.amount;
                        }
                        
                        hashmap_put(map, pair.word, &word_count);
                    }

                    pair.word[word_end] = temp->chunk[j];
                    word_end++;
                }
                else{   // is a number
                    if(word_end > 0){
                        pair.word[word_end] = '\0';
                        word_end = 0;
                    }
                    value_as_string[value_end] = temp->chunk[j];
                    value_end++;
                }
            }

            // add last pair
            value_as_string[value_end] = '\0';
            value_end = 0;


            pair.amount = atoi(value_as_string);
            int word_count = pair.amount;
            if(hashmap_contains(map, pair.word)){
                hashmap_get(map, pair.word, &word_count);
                word_count += pair.amount;
            }
            hashmap_put(map, pair.word, &word_count);
                
            // add worker back to queue
            list_insert_back(workers_waiting_queue, &worker_index);

            free(temp);
        }
    }

    
    // kill all workers with RIP
    for(unsigned int i=0; i<amount_of_ports; i++){
        worker_data *temp = (worker_data *) calloc(1, sizeof(worker_data));
        if(!temp){
            printf("Could not allocate worker data as thread argument.\n");
            exit(1);
        }
        temp->port = ports[i];
        temp->chunk[0] = '\0';
        temp->context = context;
        temp->command = RIP;
        pthread_create((pthread_t *)&worker_handlers[i], NULL, handle_worker_thread, (void *)temp);
    }

    for(unsigned int i=0; i<amount_of_ports; i++){
        worker_data *temp = NULL;
        pthread_join((pthread_t)worker_handlers[i], (void **)&temp);
        free(temp);
    }

    // cleanup
    list_destroy(workers_running_queue);
    list_destroy(workers_waiting_queue);
    list_destroy(task_queue);
    zmq_ctx_destroy(context);

    // generate output
    hashmap_to_linked_list(map);
    list_merge_sort(result, compare_pairs);
    print_result_to_stdout();

    // more cleanup
    list_destroy(result);
    hashmap_destroy(map);
    return 0;
}
