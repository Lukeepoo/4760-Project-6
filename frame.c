// frame.c
#include "frame.h"

void initializeFrames(Frame frames[]) {
    for (int i = 0; i < FRAME_COUNT; i++) {
        frames[i].occupied = 0;
        frames[i].processIndex = -1;
        frames[i].pageNumber = -1;
        frames[i].dirty = 0;
        frames[i].lastUsedSec = 0;
        frames[i].lastUsedNano = 0;
    }
}
