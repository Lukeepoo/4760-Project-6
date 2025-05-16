// oss.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include "clock.h"
#include "message.h"
#include "process_table.h"
#include "frame.h"
#include "memory.h"

#define MAX_CHILDREN 18
#define TOTAL_PROCESSES 100
#define TABLE_OUTPUT_INTERVAL 1000000000
#define DISK_IO_TIME_NS 14000000

SystemClock *simClock;
int clock_shmid;

int msgid;
int *msgidPtr;
int msg_shmid;

ProcessTable *procTable;
int proc_shmid;

Frame *frames;
int frame_shmid;

typedef struct {
    int processIdx;
    int page;
    int isWrite;
    unsigned int arrivalSec;
    unsigned int arrivalNano;
} BlockedRequest;

BlockedRequest blockedQueue[100];
int blockedCount = 0;

FILE *logFile;
volatile sig_atomic_t shutdown_flag = 0;
int totalChildrenCreated = 0;
int currentChildren = 0;
int memoryAccesses = 0;
int pageFaults = 0;

void sigHandler(int sig) { shutdown_flag = 1; }

void cleanup() {
    fprintf(logFile, "\nFinal Statistics:\n");
    fprintf(logFile, "Memory Accesses: %d\n", memoryAccesses);
    fprintf(logFile, "Page Faults: %d\n", pageFaults);
    fclose(logFile);

    shmdt(simClock);
    shmdt(procTable);
    shmdt(frames);
    shmdt(msgidPtr);
    shmctl(clock_shmid, IPC_RMID, NULL);
    shmctl(proc_shmid, IPC_RMID, NULL);
    shmctl(frame_shmid, IPC_RMID, NULL);
    shmctl(msg_shmid, IPC_RMID, NULL);
    msgctl(msgid, IPC_RMID, NULL);
}

void launchChild() {
    pid_t pid = fork();
    if (pid == 0) {
        char arg1[20];
        sprintf(arg1, "%d", msg_shmid);
        execl("./user_proc", "./user_proc", arg1, NULL);
        exit(1);
    } else if (pid > 0) {
        addProcess(procTable, pid);
        totalChildrenCreated++;
        currentChildren++;
    }
}

void printMemoryLayout() {
    fprintf(logFile, "Current memory layout at time %d:%d is:\n", simClock->seconds, simClock->nanoseconds);
    fprintf(logFile, "Occupied DirtyBit LastRefS LastRefNano\n");
    for (int i = 0; i < FRAME_COUNT; i++) {
        Frame *f = &frames[i];
        fprintf(logFile, "Frame %d: %s %d %d %d\n", i,
            f->occupied ? "Yes" : "No", f->dirty, f->lastUsedSec, f->lastUsedNano);
    }
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (procTable->table[i].alive) {
            fprintf(logFile, "P%d page table: [", i);
            for (int j = 0; j < 32; j++) {
                fprintf(logFile, "%d ", procTable->table[i].pageTable[j]);
            }
            fprintf(logFile, "]\n");
        }
    }
}

void handleBlockedQueue() {
    for (int i = 0; i < blockedCount; i++) {
        BlockedRequest *br = &blockedQueue[i];
        unsigned int elapsedSec = simClock->seconds - br->arrivalSec;
        unsigned int elapsedNano = simClock->nanoseconds - br->arrivalNano;
        if (elapsedSec > 0 || elapsedNano >= DISK_IO_TIME_NS) {
            MemoryRequest req = {br->page * 1024, br->isWrite};
            int pf;
            int frame = handleMemoryRequest(br->processIdx, req, frames, procTable->table, simClock, &pf);

            fprintf(logFile, "oss: Clearing frame %d and swapping in P%d page %d\n",
                    frame, br->processIdx, br->page);

            if (frames[frame].dirty) {
                fprintf(logFile, "oss: Dirty bit of frame %d set, adding additional time to the clock\n", frame);
            }

            fprintf(logFile, "oss: Indicating to P%d that %s has happened to address %d\n",
                    br->processIdx, br->isWrite ? "write" : "read", req.address);

            Message response;
            response.mtype = procTable->table[br->processIdx].pid;
            response.requestType = MEM_ACCESS;
            response.address = req.address;
            response.isWrite = req.isWrite;
            msgsnd(msgid, &response, sizeof(Message) - sizeof(long), 0);

            for (int j = i; j < blockedCount - 1; j++) {
                blockedQueue[j] = blockedQueue[j + 1];
            }
            blockedCount--;
            i--;
        }
    }
}

int main() {
    signal(SIGINT, sigHandler);
    signal(SIGALRM, sigHandler);
    alarm(5);

    logFile = fopen("logfile.txt", "w");

    key_t key = ftok(".", 65);
    clock_shmid = shmget(key, sizeof(SystemClock), IPC_CREAT | 0666);
    simClock = (SystemClock *)shmat(clock_shmid, NULL, 0);
    simClock->seconds = 0;
    simClock->nanoseconds = 0;

    proc_shmid = shmget(IPC_PRIVATE, sizeof(ProcessTable), IPC_CREAT | 0666);
    procTable = (ProcessTable *)shmat(proc_shmid, NULL, 0);
    initializeProcessTable(procTable);

    frame_shmid = shmget(IPC_PRIVATE, sizeof(Frame) * FRAME_COUNT, IPC_CREAT | 0666);
    frames = (Frame *)shmat(frame_shmid, NULL, 0);
    initializeFrames(frames);

    msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    msg_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    msgidPtr = (int *)shmat(msg_shmid, NULL, 0);
    *msgidPtr = msgid;

    unsigned int lastPrintTime = 0;

    while (!shutdown_flag && (totalChildrenCreated < TOTAL_PROCESSES || currentChildren > 0)) {
        incrementClock(simClock, 100000);

        if (currentChildren < MAX_CHILDREN && totalChildrenCreated < TOTAL_PROCESSES) {
            launchChild();
        }

        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            removeProcess(procTable, pid);
            currentChildren--;
        }

        Message msg;
        if (msgrcv(msgid, &msg, sizeof(Message) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            int idx = -1;
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (procTable->table[i].pid == msg.mtype) {
                    idx = i;
                    break;
                }
            }

            if (idx != -1 && msg.requestType == MEM_ACCESS) {
                memoryAccesses++;
                fprintf(logFile, "oss: P%d requesting %s of address %d at time %d:%d\n",
                        idx, msg.isWrite ? "write" : "read", msg.address,
                        simClock->seconds, simClock->nanoseconds);

                MemoryRequest req = {msg.address, msg.isWrite};
                int pageFault = 0;
                int frame = handleMemoryRequest(idx, req, frames, procTable->table, simClock, &pageFault);

                if (pageFault) {
                    pageFaults++;
                    fprintf(logFile, "oss: Address %d is not in a frame, page fault\n", msg.address);
                    fprintf(logFile, "oss: Blocking P%d and queuing request for disk I/O at time %d:%d\n",
                            idx, simClock->seconds, simClock->nanoseconds);

                    blockedQueue[blockedCount++] = (BlockedRequest){idx, msg.address / 1024, msg.isWrite,
                                                                    simClock->seconds, simClock->nanoseconds};
                } else {
                    fprintf(logFile, "oss: Address %d in frame %d, %s data to P%d at time %d:%d\n",
                            msg.address, frame, msg.isWrite ? "writing" : "giving",
                            idx, simClock->seconds, simClock->nanoseconds);

                    Message response;
                    response.mtype = msg.mtype;
                    response.requestType = MEM_ACCESS;
                    response.address = msg.address;
                    response.isWrite = msg.isWrite;
                    msgsnd(msgid, &response, sizeof(Message) - sizeof(long), 0);
                }
            } else if (msg.requestType == TERMINATE) {
                fprintf(logFile, "oss: Process P%d terminating at time %d:%d\n",
                        idx, simClock->seconds, simClock->nanoseconds);
                fprintf(logFile, "oss: Released frames for P%d: ", idx);
                for (int i = 0; i < FRAME_COUNT; i++) {
                    if (frames[i].occupied && frames[i].processIndex == idx) {
                        fprintf(logFile, "%d ", i);
                    }
                }
                fprintf(logFile, "\n");

                removeProcess(procTable, msg.mtype);
                currentChildren--;
            }
        }

        handleBlockedQueue();

        if (simClock->nanoseconds - lastPrintTime >= TABLE_OUTPUT_INTERVAL) {
            printMemoryLayout();
            lastPrintTime = simClock->nanoseconds;
        }
    }

    cleanup();
    return 0;
}
