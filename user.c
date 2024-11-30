#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "common.h"

// Function to generate memory requests
void simulate_memory_request(char* request_buffer) {
    int page = rand() % 32; // Random page number (0-31)
    int offset = rand() % 1024; // Random offset (0-1023)
    char operation = (rand() % 10 < 7) ? 'R' : 'W'; // Bias towards reads (70%)
    snprintf(request_buffer, 100, "%c %d (Page %d, Offset %d)", operation, page * 1024 + offset, page, offset);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <child_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int child_id = atoi(argv[1]); // Get child ID from arguments
    srand(time(NULL) ^ getpid()); // Seed RNG uniquely for each process

    // Attach to the message queue
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    struct message msg;
    for (int i = 0; i < REQUEST_COUNT; i++) {
        char request[100];
        simulate_memory_request(request);
        snprintf(msg.mtext, sizeof(msg.mtext), "Child %d: %s", child_id, request);
        msg.mtype = 1;

        if (msgsnd(msgid, &msg, sizeof(msg.mtext), 0) == -1) {
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }

        sleep(1); // Simulate processing delay
    }

    return 0;
}
