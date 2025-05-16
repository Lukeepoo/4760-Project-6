// process_table.h
#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#define MAX_PROCESSES 18

typedef struct {
    int pid;
    int alive;
    int pageTable[32]; // 32 entries for 32k process memory
} PCB;

typedef struct {
    PCB table[MAX_PROCESSES];
} ProcessTable;

void initializeProcessTable(ProcessTable *procTable);
void addProcess(ProcessTable *procTable, int pid);
void removeProcess(ProcessTable *procTable, int pid);

#endif
