#include "./encoder.h"
#include <string.h>
#include <assert.h>


/* these are the valid types of messages that can be sent
 * map - map auf payload ausführen
 * red - reduce auf payload ausführen
 * rip - worker herunterfahren
 */
char types[3][4] = {"map", "red", "rip"};

// yes, this whole encode from and to worker thing is ugly, but I have no other choice, since the worker might receive
// words like ripeness at the start of a string, and shall not interpret it as a "kill" command

// returns 0 on success
int encode_msg_to_worker(char msg_buff[], char payload[], MSG_TYPE type){
    assert(msg_buff);
    assert(payload);
    
    if(type==INVALID || type==EMPTY)
        return 1;
    
    strcpy(msg_buff, (char *)&types[type]);               // this works because of the way enums are defined
    strcpy(&msg_buff[3], payload);
    return 0;
}

// returns 0 on success
int encode_msg(char msg_buff[], char payload[], MSG_TYPE type){
    assert(msg_buff);
    assert(payload);
    
    if(type==MAP || type==RED || type==INVALID)
        return 1;
    
    if(type == RIP){
        strcpy(msg_buff, (char*)&types[2]);
        return 0;
    }

    // type == empty
    strcpy(msg_buff, payload);
    return 0;
}

MSG_TYPE decode_msg(char msg_buff[], char payload[]){
    assert(msg_buff);
    assert(payload);

    if(msg_buff[0] == '\0'){
        return INVALID;
    }

    for(MSG_TYPE type=MAP; type<EMPTY; type++){
        if(!strncmp(msg_buff, (char *)&types[type], 3)){     // this checks the first 3 chars of the string msg_buff for all valid types
            strcpy(payload, &msg_buff[3]);
            return type;
        }
    }

    return INVALID;
}

// I have to do this stupid shit, because of the way you want me to implement the protocol >:[
// jokes aside, I dislike this very much and I'd rather send a NUL or something as a 'command',
// because that would be a much cleaner way to do so (consistent spacing of packages),
// but then I won't pass the your tests...
MSG_TYPE decode_msg_from_worker(char msg_buff[], char payload[]){
    assert(msg_buff);
    assert(payload);

    if(msg_buff[0] == '\0'){
        payload[0] = '\0';
        return EMPTY;
    }

    // we assume a msg_buff size of at lest 4 (3 chars for "rip" + 1 for 'NUL') (AT LEAST)
    // rip reply must be sent with nothing but a NUL behind the "rip", while a word like ripeness has more chars behind the "rip"
    if(!strncmp(msg_buff, (char *)&types[2], 3) && msg_buff[3] == '\0'){
        strcpy(payload, &msg_buff[3]);
        return RIP;
    }

    // just assume, that if not RIP, the payload carries a payload without a command
    // everything else would be an absolute nightmare to implement, and those are the only
    // two commands a worker should be sending to the distributor
    // -> no checking for INVALID types here
    strcpy(payload, msg_buff);
    return EMPTY;
}