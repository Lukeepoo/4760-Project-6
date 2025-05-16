// message.h
#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct {
    long mtype;        // PID of the sender
    int requestType;   // 0 = Memory Access, 1 = Terminate
    int address;
    int isWrite;
} Message;

#define MEM_ACCESS 0
#define TERMINATE 1

#endif
