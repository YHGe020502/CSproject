#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "queue.h"
#include "allocation.h"

enum {
    UNREADY,
    READY,
    RUNNING,
    FINISHED
};
enum {
    SJF,
    RR
};
enum {
    INFINITE,
    BEST_FIT
};

typedef struct Proc {
    unsigned int time_arrived;
    char name[9];
    unsigned int service_time;
    unsigned int remaining_time;
    int memory_required;
    int state;
    int address;
    int fd_stdin[2];
    int fd_stdout[2];
    pid_t pid;
} Proc;

Proc* ProcCreate(unsigned int time_arrived, char *name, unsigned int service_time, unsigned int memory_required, int state);
void ProcDestroy(Proc *proc);
void SchedulerCreate(char*filename);
void SchedulerDestroy();
void PrintState(int state);
void PrintProcessState(int state);
void SchedulerRun(int scheduler_mode, int memory_strategy_mode, int quantum);
void SchedulerSJFRun(int quantum, int memory_strategy_mode);
void SchedulerRRRun(int quantum, int memory_strategy_mode);
void ProcessCreate(Proc *proc);
void ProcessSuspend(Proc *proc);
void ProcessResume(Proc *proc);
void ProcessTerminate(Proc *proc);
unsigned int GetBigEndien(unsigned int num);
void PerformanceStatistics();

#endif