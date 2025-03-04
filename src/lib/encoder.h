#pragma once

//! WARNING: Both functions assume that the supplied buffer size is sufficient
// These functions handle the encoding and decoding of zmq messages.

// this is being used to describe the type of function that should be or has been encoded
typedef enum{
    MAP,
    RED,
    RIP,
    EMPTY,      // for response of worker thread
    INVALID     // for error handling
}MSG_TYPE;

/* Both functions share the following parameters:
 * char msg_buff[] - buffer where the encoded zmq message will be/ has been stored
 * char payload[]  - buffer for the contents/ data of the message itself
 * 
 * the buffer where the needed contents are being supplied, must be POSIX conform ('\0' at the end)
 */
int encode_msg(char msg_buff[], char payload[], MSG_TYPE type);                 // returns 0 on success
int encode_msg_to_worker(char msg_buff[], char payload[], MSG_TYPE type);       // returns 0 on success

MSG_TYPE decode_msg(char msg_buff[], char payload[]);

// I have to do this stupid shit, because of the way you want me to implement the protocol >:[
// jokes aside, I dislike this very much and I'd rather send a NUL or something as a 'command',
// because that would be a much cleaner way to do so (consistent spacing of packages),
// but then, I wouldn't pass the tests, so here we are...
MSG_TYPE decode_msg_from_worker(char msg_buff[], char payload[]);