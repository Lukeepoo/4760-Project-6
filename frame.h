// frame.h
#ifndef FRAME_H
#define FRAME_H

#define FRAME_COUNT 256

typedef struct {
    int occupied;
    int processIndex;    // Index in ProcessTable
    int pageNumber;      // Page number in process's page table
    int dirty;
    unsigned int lastUsedSec;
    unsigned int lastUsedNano;
} Frame;

void initializeFrames(Frame frames[]);

#endif
