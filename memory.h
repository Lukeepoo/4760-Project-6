// memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include "clock.h"
#include "frame.h"
#include "process_table.h"

// Represents a single memory access request from a user process
typedef struct {
    int address;       // The actual memory address requested
    int isWrite;       // 1 if it's a write request, 0 for read
} MemoryRequest;

// Handles a memory request and returns the frame number used.
// Sets *pageFaultOccurred to 1 if a page fault occurred, otherwise 0.
int handleMemoryRequest(int processIdx, MemoryRequest req, Frame frames[], PCB procTable[], SystemClock *clock, int *pageFaultOccurred);

// Finds the least recently used frame (or a free one if available)
int findLRUFrame(Frame frames[], SystemClock *clock);

// Updates the LRU timestamp of a frame to the current simulated clock time
void updateLRU(Frame *frame, SystemClock *clock);

#endif
