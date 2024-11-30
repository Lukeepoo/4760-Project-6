#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "common.h"

// Shared clock
unsigned long long* shared_clock;
int shmid; // Shared memory ID
int msgid; // Message queue ID

// Frame table structure
struct frame {
    int occupied;
    int process_id;
    int page_number;
    unsigned long long last_access_time;
};

struct frame frame_table[FRAME_COUNT];

void initialize_frame_table() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        frame_table[i].occupied = 0;
        frame_table[i].process_id = -1;
        frame_table[i].page_number = -1;
        frame_table[i].last_access_time = 0;
    }
}

void cleanup() {
    if (shmdt(shared_clock) == -1) {
        perror("shmdt failed");
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
    }
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl failed");
    }
    printf("Shared memory and message queue cleaned up.\n");
}

void signal_handler(int sig) {
    printf("Signal %d received. Cleaning up and exiting.\n", sig);
    cleanup();
    exit(0);
}

void dump_frame_table(FILE* logfile) {
    fprintf(logfile, "Frame Table State:\n");
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (frame_table[i].occupied) {
            fprintf(logfile, "Frame %d: Process %d, Page %d, Last Access %llu\n",
                    i, frame_table[i].process_id,
                    frame_table[i].page_number,
                    frame_table[i].last_access_time);
        } else {
            fprintf(logfile, "Frame %d: Free\n", i);
        }
    }
    fprintf(logfile, "-------------------------------------\n");
}

void handle_memory_request(int process_id, int page_number, FILE* logfile) {
    int frame_index = -1;

    // Check if the page is already in a frame
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (frame_table[i].occupied && frame_table[i].process_id == process_id && frame_table[i].page_number == page_number) {
            frame_index = i;
            break;
        }
    }

    if (frame_index == -1) {
        // Page fault, find a free frame or evict using LRU
        fprintf(logfile, "Page fault: Process %d, Page %d\n", process_id, page_number);

        for (int i = 0; i < FRAME_COUNT; i++) {
            if (!frame_table[i].occupied) {
                frame_index = i;
                break;
            }
        }

        if (frame_index == -1) {
            // No free frame, evict the least recently used frame
            unsigned long long oldest_time = *shared_clock;
            for (int i = 0; i < FRAME_COUNT; i++) {
                if (frame_table[i].last_access_time < oldest_time) {
                    oldest_time = frame_table[i].last_access_time;
                    frame_index = i;
                }
            }

            fprintf(logfile, "Evicting frame %d (Process %d, Page %d)\n",
                    frame_index, frame_table[frame_index].process_id,
                    frame_table[frame_index].page_number);
        }

        // Allocate the frame
        frame_table[frame_index].occupied = 1;
        frame_table[frame_index].process_id = process_id;
        frame_table[frame_index].page_number = page_number;
        frame_table[frame_index].last_access_time = *shared_clock;

        fprintf(logfile, "Assigned Process %d, Page %d to Frame %d\n",
                process_id, page_number, frame_index);
    } else {
        // Page hit
        frame_table[frame_index].last_access_time = *shared_clock;
        fprintf(logfile, "Page hit: Process %d, Page %d in Frame %d\n",
                process_id, page_number, frame_index);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    FILE* logfile = fopen("logfile.txt", "w");
    if (!logfile) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    // Allocate shared memory
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    shared_clock = (unsigned long long*)shmat(shmid, NULL, 0);
    if (shared_clock == (void*)-1) {
        perror("shmat failed");
        cleanup();
        exit(EXIT_FAILURE);
    }

    *shared_clock = 0; // Initialize shared clock
    printf("Shared memory initialized at address: %p\n", (void*)shared_clock);

    // Create message queue
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget failed");
        cleanup();
        exit(EXIT_FAILURE);
    }

    printf("Message queue created.\n");

    // Initialize frame table
    initialize_frame_table();

    // Fork and execute child processes
    for (int i = 0; i < MAX_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            cleanup();
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            char child_id[10];
            snprintf(child_id, sizeof(child_id), "%d", i);
            execl("./user", "./user", child_id, (char*)NULL);

            // If execl fails
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process handles messages
    struct message msg;
    for (int i = 0; i < MAX_CHILDREN * REQUEST_COUNT; i++) {
        if (msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
            perror("msgrcv failed");
            cleanup();
            exit(EXIT_FAILURE);
        }

        printf("Parent received message: %s\n", msg.mtext);

        // Simplified parsing for memory request
        int process_id = i / REQUEST_COUNT; // Simulate process ID
        int page_number = rand() % PAGE_COUNT; // Simulate page number

        handle_memory_request(process_id, page_number, logfile);
        (*shared_clock) += 100; // Increment clock

        if (i % 10 == 0) { // Periodically dump frame table
            dump_frame_table(logfile);
        }
    }

    // Wait for child processes to complete
    for (int i = 0; i < MAX_CHILDREN; i++) {
        wait(NULL);
    }

    cleanup();
    fclose(logfile);
    return 0;
}
