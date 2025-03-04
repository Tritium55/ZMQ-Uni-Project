// This is the client side of this project

#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include "../lib/encoder.h"
#include "../lib/hashmap.h"

#define MSG_LEN 1500

static inline bool is_alpha(char c){
    return (c>='a' && c<='z') || (c>='A' && c<='Z');
}

static inline int to_lower(int c){
    if(c>='A' && c<='Z')
        return c + 32;
    return c;
}

static void append_one_to_result(char *buffer, void *key, void *value){
    assert(buffer);
    assert(key);
    assert(value);

    char *word = (char *) key;
    int word_count = *(int *) value;

    if(snprintf(buffer, MSG_LEN, "%s", word) < 0){
        fprintf(stderr, "Could not append key-value pair to result buffer.\n");
        exit(1);
    }

    for(int i=0; i<word_count; i++){
        strcat(buffer, "1");
    }
}

static void append_number_to_result(char *buffer, void *key, void *value){
    assert(buffer);
    assert(key);
    assert(value);

    char *word = (char *) key;
    int word_count = *(int *) value;

    if(snprintf(buffer, MSG_LEN, "%s", word) < 0){
        fprintf(stderr, "Could not append key-value pair to result buffer.\n");
        exit(1);
    }

    char number[10];        // should be enough
    snprintf(number, sizeof(number), "%d", word_count);
    strcat(buffer, number);
}

// expects a buffer as result, so that the result can be copied into it
static void map(char *string, char *result){
    assert(string);
    assert(result);

    if(string[0] == '\0'){
        result[0] = '\0';
        return;
    }

    hashmap *map = hashmap_init(10, sizeof(char) * MSG_LEN, sizeof(int), NULL, NULL);
    char temp[MSG_LEN];
    int word_end = 0;

    for(int i=0; string[i] != '\0'; i++){
        if(is_alpha(string[i])){
            temp[word_end] = to_lower(string[i]);
            word_end++;
        }else if(word_end>0){
            temp[word_end] = '\0';
            word_end = 0;

            int word_count = 1;
            if(hashmap_contains(map, temp)){
                hashmap_get(map, temp, &word_count);
                word_count++;
            }
            
            hashmap_put(map, temp, &word_count);
        }
    }

    // last word might not have ended with a space
    if(word_end > 0){
        temp[word_end] = '\0';
        int word_count = 1;
        if(hashmap_contains(map, temp)){
            hashmap_get(map, temp, &word_count);
            word_count++;
        }
        hashmap_put(map, temp, &word_count);
    }

    hashmap_to_string(map, result, append_one_to_result);

    hashmap_destroy(map);
    return;
}

static void reduce(char *string, char *result){
    assert(string);
    assert(result);

    hashmap *map = hashmap_init(10, sizeof(char) * MSG_LEN, sizeof(int), NULL, NULL);

    char temp[MSG_LEN] = {0};
    int word_end = 0;
    int current_amount_of_ones = 0;
    for(int i=0; string[i] != '\0'; i++){
        if(is_alpha(string[i])){
            if(current_amount_of_ones>0 && word_end>0){    // now encountered the next word
                temp[word_end] = '\0';
                
                // this process could be optimized a little by adding a hashmap_increase_value function
                int word_count = 0;
                if(hashmap_contains(map, temp)){
                    hashmap_get(map, temp, &word_count);
                }

                word_count += current_amount_of_ones;
                hashmap_put(map, temp, &word_count);
                word_end = 0;
                current_amount_of_ones = 0;
                memset(temp, 0, sizeof(temp));
            }

            // copy current word to temp
            temp[word_end] = string[i];
            word_end++;
        }
        else if(string[i] == '1')
            current_amount_of_ones++;

        else{
            fprintf(stderr, "Invalid character in string on reduce function call.\n");
            exit(1);
        }
    }

    // doing the same stuff for the last word in the string
    if(current_amount_of_ones>0 && word_end>0){
        temp[word_end] = '\0';
                
        // this process could be optimized a little by adding a hashmap_increase_value function
        int word_count = 0;
        if(hashmap_contains(map, temp)){
            hashmap_get(map, temp, &word_count);
        }

        word_count += current_amount_of_ones;
        hashmap_put(map, temp, &word_count);
    }

    // again some POSIX conform bs
    result[0] = '\0';
    hashmap_to_string(map, result, append_number_to_result);

    hashmap_destroy(map);
}

typedef struct{
    void *context;
    int port;
}worker_data;

void *worker_thread(void *data){
    assert(data);
    worker_data *worker = (worker_data *) data;
    assert(worker->context);
    assert(worker->port>0);

    void *worker_socket = zmq_socket(worker->context, ZMQ_REP);

    // convert port number to string and copy to buffer
    char port_buff[30];
    if(snprintf(port_buff, sizeof(port_buff), "tcp://*:%d", worker->port) < 0){
        fprintf(stderr, "Port number could not be copied.\n");
        pthread_exit(NULL);
    }
    int rc = zmq_bind(worker_socket, port_buff);
    if(rc != 0){
        fprintf(stderr, "Could not connect to port %d (ZMQ error): %s\n", worker->port, zmq_strerror(zmq_errno()));
        zmq_close(worker_socket);
        pthread_exit(NULL);
    }

    while(true){
        // handle request, action, and response
        char msg_buff[MSG_LEN] = {0};
        char payload_buff[MSG_LEN - 3] = {0}; 
        char result_buff[MSG_LEN] = {0};
        zmq_recv(worker_socket, msg_buff, MSG_LEN, 0);
        
        MSG_TYPE type = decode_msg(msg_buff, payload_buff);
        switch(type){
            case MAP:
                //fprintf(stderr, "\nMAP INPUT:\n%s\n", payload_buff);
                map(payload_buff, result_buff);
                //fprintf(stderr, "\nMAP RESULT:\n%s\n\n\n", result_buff);      // todo remove print function calls
                memset(msg_buff, 0, sizeof(msg_buff));
                if(encode_msg(msg_buff, result_buff, EMPTY) != 0){
                    fprintf(stderr, "Couldn't encode map message.\n");
                    goto kill_worker_thread;
                }
                zmq_send(worker_socket, msg_buff, strlen(msg_buff)+1, 0);
                break;

            case RED:
                // todo: remove print calls
                //fprintf(stderr, "\n\nREDUCE INPUT:\n%s\n", payload_buff);
                reduce(payload_buff, result_buff);
                //fprintf(stderr, "REDUCE RESULT:\n%s\n\n\n", result_buff);
                memset(msg_buff, 0, sizeof(msg_buff));
                if(encode_msg(msg_buff, result_buff, EMPTY) != 0){
                    fprintf(stderr, "Couldn't encode red message.\n");
                    goto kill_worker_thread;
                }
                zmq_send(worker_socket, msg_buff, strlen(msg_buff)+1, 0);
                break;
            
            case RIP:
                if(encode_msg(msg_buff, "", RIP) != 0){
                    fprintf(stderr, "Couldn't encode rip message.\n");
                    goto kill_worker_thread;
                }
                zmq_send(worker_socket, msg_buff, 4, 0);
                goto kill_worker_thread;
                break;
            
            default:
                fprintf(stderr, "No command found within the received message. Listening for next message\n");
                break;
        }
    }

    kill_worker_thread: ;      // not the cleanest way to do this, but it works

    zmq_close(worker_socket);
    pthread_exit(NULL);
}

int main(int argc, char **argv){
    unsigned int amount_of_ports = 0;
    if(argc==1){
        fprintf(stderr, "Not enough arguments: %d.\n", argc);
        return 1;
    }
    
    amount_of_ports = (unsigned int) argc - 1;     // subtract 1 because program name is the first parameter
    assert(amount_of_ports>0);

    void *context = zmq_ctx_new();
    // parse all port numbers
    pthread_t workers[(const unsigned int)amount_of_ports];
    worker_data worker_arguments[(const unsigned int)amount_of_ports];
    for(unsigned int i=0; i<amount_of_ports; i++){
        worker_arguments[i].context = context;
        worker_arguments[i].port = atoi(argv[i+1]);
        pthread_create((pthread_t *)&workers[i], NULL, worker_thread, &worker_arguments[i]);
    }

    for(unsigned int i=0; i<amount_of_ports; i++){
        pthread_join((pthread_t)workers[i], NULL);
    }

    zmq_ctx_destroy(context);
    return 0;
}
