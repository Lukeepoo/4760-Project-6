// user_proc.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "clock.h"
#include "message.h"

int clock_shmid;
SystemClock *simClock;
int *msgidPtr;
int msgid;

volatile sig_atomic_t terminate_flag = 0;

void sigHandler(int sig) {
    terminate_flag = 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "user_proc: Missing msgid shm arg\n");
        exit(1);
    }

    signal(SIGINT, sigHandler);

    key_t key = ftok(".", 65);
    clock_shmid = shmget(key, sizeof(SystemClock), 0666);
    simClock = (SystemClock *)shmat(clock_shmid, NULL, 0);

    int msg_shmid = atoi(argv[1]);
    msgidPtr = (int *)shmat(msg_shmid, NULL, 0);
    msgid = *msgidPtr;

    int accesses = 0;
    while (!terminate_flag && accesses < 1000) {
        Message msg;
        msg.mtype = getpid();
        msg.requestType = MEM_ACCESS;
        int page = rand() % 32;
        int offset = rand() % 1024;
        msg.address = page * 1024 + offset;
        msg.isWrite = rand() % 10 == 0;

        msgsnd(msgid, &msg, sizeof(Message) - sizeof(long), 0);

        Message response;
        msgrcv(msgid, &response, sizeof(Message) - sizeof(long), getpid(), 0);

        accesses++;
        if (accesses >= 1000 || (rand() % 100 < 5)) {
            Message termMsg;
            termMsg.mtype = getpid();
            termMsg.requestType = TERMINATE;
            msgsnd(msgid, &termMsg, sizeof(Message) - sizeof(long), 0);
            break;
        }

        usleep(1000);
    }

    shmdt(simClock);
    shmdt(msgidPtr);
    return 0;
}
