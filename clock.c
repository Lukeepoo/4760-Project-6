// clock.c
// Clock increment implementation

#include "clock.h"

// Increment simulated clock by ns nanoseconds
void incrementClock(SystemClock *clock, unsigned int ns) {
    clock->nanoseconds += ns;
    if (clock->nanoseconds >= 1000000000) {
        clock->seconds++;
        clock->nanoseconds -= 1000000000;
    }
}
