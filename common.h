#ifndef COMMON_H
#define COMMON_H

#define MSG_KEY 0x54321
#define REQUEST_COUNT 3
#define MAX_CHILDREN 5
#define FRAME_COUNT 256
#define PAGE_COUNT 32

// Add these missing definitions:
#define SHM_KEY 0x12345
#define SHM_SIZE sizeof(unsigned long long) // Shared memory size for the clock

struct message {
    long mtype;
    char mtext[150];
};

#endif
