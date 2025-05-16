// clock.h
// Clock header

#ifndef CLOCK_H
#define CLOCK_H

typedef struct {
    unsigned int seconds;
    unsigned int nanoseconds;
} SystemClock;

// Increment clock function
void incrementClock(SystemClock *clock, unsigned int ns);

#endif
