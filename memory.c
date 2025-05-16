// memory.c
#include "memory.h"
#include <limits.h>
#include <stdio.h>

// Updates a frame's last-used time to current system clock
void updateLRU(Frame *frame, SystemClock *clock) {
    frame->lastUsedSec = clock->seconds;
    frame->lastUsedNano = clock->nanoseconds;
}

// Finds the least recently used frame, or a free one if available
int findLRUFrame(Frame frames[], SystemClock *clock) {
    int victim = -1;
    unsigned int minSec = UINT_MAX, minNano = UINT_MAX;

    for (int i = 0; i < FRAME_COUNT; i++) {
        if (!frames[i].occupied) return i; // Use free frame immediately

        if (frames[i].lastUsedSec < minSec ||
            (frames[i].lastUsedSec == minSec && frames[i].lastUsedNano < minNano)) {
            minSec = frames[i].lastUsedSec;
            minNano = frames[i].lastUsedNano;
            victim = i;
        }
    }
    return victim;
}

// Handles a single memory access request (read or write)
// Performs LRU replacement if necessary and updates page tables
int handleMemoryRequest(int processIdx, MemoryRequest req, Frame frames[], PCB procTable[], SystemClock *clock, int *pageFaultOccurred) {
    int address = req.address;
    int pageNum = address / 1024;

    int frameNum = procTable[processIdx].pageTable[pageNum];
    *pageFaultOccurred = 0;

    // Page fault if no frame mapped or mismatch
    if (frameNum == -1 || !frames[frameNum].occupied ||
        frames[frameNum].processIndex != processIdx ||
        frames[frameNum].pageNumber != pageNum) {

        *pageFaultOccurred = 1;
        int victim = findLRUFrame(frames, clock);

        // Remove old mapping if victim is occupied
        if (frames[victim].occupied) {
            procTable[frames[victim].processIndex].pageTable[frames[victim].pageNumber] = -1;
        }

        // Map new frame
        frames[victim].occupied = 1;
        frames[victim].processIndex = processIdx;
        frames[victim].pageNumber = pageNum;
        frames[victim].dirty = req.isWrite;
        updateLRU(&frames[victim], clock);
        procTable[processIdx].pageTable[pageNum] = victim;

        return victim;
    } else {
        // No page fault, just update access time and possibly dirty bit
        updateLRU(&frames[frameNum], clock);
        if (req.isWrite)
            frames[frameNum].dirty = 1;
        return frameNum;
    }
}
