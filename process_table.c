// process_table.c
// Process table management implementations

#include "process_table.h"
#include <string.h>

// Initialize all process table entries
void initializeProcessTable(ProcessTable *procTable) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        procTable->table[i].pid = -1;
        procTable->table[i].alive = 0;
        for (int j = 0; j < 32; j++) {
            procTable->table[i].pageTable[j] = -1;  // Initialize all page table entries to -1 (unmapped)
        }
    }
}

// Add a new process
void addProcess(ProcessTable *procTable, int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (procTable->table[i].alive == 0) {
            procTable->table[i].pid = pid;
            procTable->table[i].alive = 1;
            for (int j = 0; j < 32; j++) {
                procTable->table[i].pageTable[j] = -1;
            }
            break;
        }
    }
}

// Mark a process as terminated
void removeProcess(ProcessTable *procTable, int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (procTable->table[i].pid == pid) {
            procTable->table[i].alive = 0;
            for (int j = 0; j < 32; j++) {
                procTable->table[i].pageTable[j] = -1;
            }
            break;
        }
    }
}
