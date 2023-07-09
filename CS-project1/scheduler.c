#include "scheduler.h"

Queue* inputQueue;
Queue* readyQueue;
unsigned int curTime;
unsigned int procRemaining;

unsigned int totalProcess;
unsigned int totalTurnaroundTime;
float totalOverhead;
float maxOverhead;

Proc* ProcCreate(unsigned int time_arrived, char *name, unsigned int service_time, unsigned int memory_required, int state) {
    Proc *proc = (Proc*)malloc(sizeof(Proc));
    proc->time_arrived = time_arrived;
    strcpy(proc->name, name);
    proc->service_time = service_time;
    proc->remaining_time = service_time;
    proc->memory_required = memory_required;
    proc->state = state;
    proc->address = -1;
    proc->pid = 0;
    return proc;
}
void ProcDestroy(Proc *proc) {
    free(proc);
}

void SchedulerCreate(char*filename) {
    FILE* fp = fopen(filename, "r");
    char line[100];
    inputQueue = QueueCreate();
    readyQueue = QueueCreate();
    curTime = 0;
    procRemaining = 0;
    totalProcess = 0;
    totalTurnaroundTime = 0;
    totalOverhead = 0;
    maxOverhead = 0;
    char name[9];
    unsigned int time_arrived, service_time;
    int memory_required;
    while (fgets(line, 100, fp) != NULL) {
        line[strlen(line) - 1] = '\0';
        sscanf(line, "%u %s %u %d", &time_arrived, name, &service_time, &memory_required);
        Proc *proc = ProcCreate(time_arrived, name, service_time, memory_required, UNREADY);
        // printf("%u %s %u %d\n", proc->time_arrived, proc->name, proc->service_time, proc->memory_required);
        QueueEnqueue(inputQueue, proc);
    }
    fclose(fp);
}

void SchedulerDestroy() {
    QueueDestroy(inputQueue);
    QueueDestroy(readyQueue);
}

void PrintState(int state) {
    if (state == RUNNING) {
        printf("RUNNING,");
    } else if (state == FINISHED) {
        printf("FINISHED,");
    } else if (state == READY) {
        printf("READY,");
    } else if (state == UNREADY) {
        printf("UNREADY,");
    }
}

void PrintProcessState(int state) {
    if (state == RUNNING) {
        printf("RUNNING-PROCESS,");
    } else if (state == FINISHED) {
        printf("FINISHED-PROCESS,");
    } else if (state == READY) {
        printf("READY-PROCESS,");
    } else if (state == UNREADY) {
        printf("UNREADY-PROCESS,");
    }
}

void SchedulerRun(int scheduler_mode, int memory_strategy_mode, int quantum) {
    if (scheduler_mode == SJF) {
        SchedulerSJFRun(quantum, memory_strategy_mode);
    } else if (scheduler_mode == RR) {
        SchedulerRRRun(quantum, memory_strategy_mode);
    }
}

void SchedulerSJFRun(int quantum, int memory_strategy_mode) {
    Proc* runningProc = NULL;
    while (procRemaining > 0 || !QueueEmpty(inputQueue) || !QueueEmpty(readyQueue)) {
        // move the proc which ready to run to the readyQueue
        Node* iter = QueueIter(inputQueue);
        while (iter != NULL) {
            Proc* proc = iter->proc;
            if (curTime >= proc->time_arrived) {
                if (memory_strategy_mode == BEST_FIT) {
                    int addr = AllocationMalloc(proc->memory_required);
                    if (addr != -1) {
                        proc->address = addr;
                        proc->state = READY;
                        printf("%u,", curTime);
                        PrintState(proc->state);
                        printf("process_name=%s,", proc->name);
                        printf("assigned_at=%d\n", proc->address);
                        QueuePriorityEnqueue(readyQueue, proc);
                        iter = QueueIterRemove(inputQueue, iter);
                        procRemaining++;
                    } else {
                        iter = iter->next;
                    }
                } else {
                    proc->state = READY;
                    QueuePriorityEnqueue(readyQueue, proc);
                    iter = QueueIterRemove(inputQueue, iter);
                    procRemaining++;
                }
            } else {
                iter = iter->next;
            }
        }
        // if no running proc, get the first proc in the readyQueue
        if (runningProc == NULL) {
            if (QueueEmpty(readyQueue)) {   // if no ready proc, continue to next cycle
                curTime += quantum;
                continue;
            }
            // new proc running
            runningProc = QueueDequeue(readyQueue);
            runningProc->state = RUNNING;
            printf("%u,", curTime);
            PrintState(runningProc->state);
            printf("process_name=%s,", runningProc->name);
            printf("remaining_time=%u\n", runningProc->remaining_time);
            ProcessCreate(runningProc);
        }
        curTime += quantum;
        // if the process is running
        if (runningProc != NULL) {
            // process finished, destory it
            if (runningProc->remaining_time <= quantum) {
                // free the memory
                if (memory_strategy_mode == BEST_FIT) {
                    AllocationFree(runningProc->address);
                }
                runningProc->state = FINISHED;
                procRemaining--;
                printf("%u,", curTime);
                PrintState(runningProc->state);
                printf("process_name=%s,", runningProc->name);
                printf("proc_remaining=%u\n", procRemaining);
                ProcessTerminate(runningProc);
                totalProcess += 1;
                unsigned int timeTurnaround = curTime - runningProc->time_arrived;
                totalTurnaroundTime += timeTurnaround;
                float timeOverhead = timeTurnaround * 1.0 / runningProc->service_time;
                totalOverhead += timeOverhead;
                if (maxOverhead < timeOverhead) {
                    maxOverhead = timeOverhead;
                }
                ProcDestroy(runningProc);
                runningProc = NULL;
            } else {
                // process not finished, continue to run
                runningProc->remaining_time -= quantum;
                ProcessResume(runningProc);
            }
        }
    }
}

void SchedulerRRRun(int quantum, int memory_strategy_mode) {
    Proc* runningProc = NULL;
    // allocate memory
    Node* iter = QueueIter(inputQueue);
    while (iter != NULL) {
        Proc* proc = iter->proc;
        if (memory_strategy_mode == BEST_FIT && curTime >= proc->time_arrived) {
            int addr = AllocationMalloc(proc->memory_required);
            if (addr != -1) {
                proc->address = addr;
                proc->state = READY;
                procRemaining++;
            }
        }
        iter = iter->next;
    }
    // simulate process running
    while (procRemaining > 0 || !QueueEmpty(inputQueue) || !QueueEmpty(readyQueue)) {
        // move the proc which ready to run to the readyQueue
        iter = QueueIter(inputQueue);
        while (iter != NULL) {
            Proc* proc = iter->proc;
            if (curTime >= proc->time_arrived) {
                if (memory_strategy_mode == BEST_FIT) {
                    if (proc->state == READY) {
                        printf("%u,", curTime);
                        PrintState(proc->state);
                        printf("process_name=%s,", proc->name);
                        printf("assigned_at=%d\n", proc->address);
                        QueueEnqueue(readyQueue, proc);
                        iter = QueueIterRemove(inputQueue, iter);
                    } else {
                        iter = iter->next;
                    }
                } else {
                    proc->state = READY;
                    QueueEnqueue(readyQueue, proc);
                    iter = QueueIterRemove(inputQueue, iter);
                    procRemaining++;
                }
            } else {
                iter = iter->next;
            }
        }
        // if process is running, stop it and move it to the readyQueue
        if (runningProc != NULL) {
            if (!QueueEmpty(readyQueue)) {
                runningProc->state = READY;
                QueueEnqueue(readyQueue, runningProc);
                ProcessSuspend(runningProc);
                runningProc = NULL;
            } else {
                ProcessResume(runningProc);
            }
        }
        // if no running proc, get the first proc in the readyQueue
        if (runningProc == NULL) {
            if (QueueEmpty(readyQueue)) {   // if no ready proc, continue to next cycle
                curTime += quantum;
                continue;
            }
            runningProc = QueueDequeue(readyQueue);
            runningProc->state = RUNNING;
            printf("%u,", curTime);
            PrintState(runningProc->state);
            printf("process_name=%s,", runningProc->name);
            printf("remaining_time=%u\n", runningProc->remaining_time);
            if (runningProc->pid == 0) {
                ProcessCreate(runningProc);
            } else {
                ProcessResume(runningProc);
            }
        }
        curTime += quantum;
        // free the memory
        if (runningProc != NULL && runningProc->remaining_time <= quantum && memory_strategy_mode == BEST_FIT) {
            AllocationFree(runningProc->address);
        }
        // allocate memory
        iter = QueueIter(inputQueue);
        while (iter != NULL) {
            Proc* proc = iter->proc;
            if (memory_strategy_mode == BEST_FIT && curTime >= proc->time_arrived) {
                int addr = AllocationMalloc(proc->memory_required);
                if (addr != -1) {
                    proc->address = addr;
                    proc->state = READY;
                    procRemaining++;
                }
            }
            iter = iter->next;
        }
        // if the process is running
        if (runningProc != NULL) {
            // process finished, destory it
            if (runningProc->remaining_time <= quantum) {
                runningProc->state = FINISHED;
                procRemaining--;
                printf("%u,", curTime);
                PrintState(runningProc->state);
                printf("process_name=%s,", runningProc->name);
                printf("proc_remaining=%u\n", procRemaining);
                ProcessTerminate(runningProc);
                totalProcess += 1;
                unsigned int timeTurnaround = curTime - runningProc->time_arrived;
                totalTurnaroundTime += timeTurnaround;
                float timeOverhead = timeTurnaround * 1.0 / runningProc->service_time;
                totalOverhead += timeOverhead;
                if (maxOverhead < timeOverhead) {
                    maxOverhead = timeOverhead;
                }
                ProcDestroy(runningProc);
                runningProc = NULL;
            } else {
                // process not finished, continue to run
                runningProc->remaining_time -= quantum;
            }
        }
    }
}

void ProcessCreate(Proc *proc) {
    unsigned char checkByte = 0;
    char arg1[100];
    char arg2[100];
    char* argv[3];
    sprintf(arg1, "./process");
    sprintf(arg2, "%s", proc->name);
    argv[0] = arg1;
    argv[1] = arg2;
    argv[2] = NULL;
    pid_t pid;
    int err;
    err = pipe(proc->fd_stdin);
    if (err == -1) {
        perror("pipe");
        exit(1);
    }
    err = pipe(proc->fd_stdout);
    if (err == -1) {
        perror("pipe");
        exit(1);
    }
    pid = fork();
    if (pid == 0) {
        // child process
        // redirect the stdin and stdout
        dup2(proc->fd_stdin[0], STDIN_FILENO);
        close(proc->fd_stdin[1]);
        dup2(proc->fd_stdout[1], STDOUT_FILENO);
        close(proc->fd_stdout[0]);
        // execute the process
        execv(argv[0], argv);

    } else {
        // parent process
        close(proc->fd_stdin[0]);
        close(proc->fd_stdout[1]);
        proc->pid = pid;
        // send the 32bit simulation time
        unsigned int simulationTime = GetBigEndien(curTime);
        err = write(proc->fd_stdin[1], &simulationTime, sizeof(simulationTime));
        if (err <= 0) {
            perror("write");
            exit(1);
        }
        // read 1 byte from the stdout of process
        err = read(proc->fd_stdout[0], &checkByte, 1);
        if (err <= 0) {
            perror("read");
            exit(1);
        }
    }
}

void ProcessSuspend(Proc *proc) {
    // send the 32bit simulation time
    int err;
    unsigned int simulationTime = GetBigEndien(curTime);
    err = write(proc->fd_stdin[1], &simulationTime, sizeof(simulationTime));
    if (err <= 0) {
        perror("write");
        exit(1);
    }
    // Send a SIGTSTP signal to process
    kill(proc->pid, SIGTSTP);
    int waitStatus;
    waitpid(proc->pid, &waitStatus, WUNTRACED);
}

void ProcessResume(Proc *proc) {
    unsigned char checkByte = 0;
    // send the 32bit simulation time
    unsigned int simulationTime = GetBigEndien(curTime);
    int err;
    err = write(proc->fd_stdin[1], &simulationTime, sizeof(simulationTime));
    if (err <= 0) {
        perror("write");
        exit(1);
    }
    // Send a SIGCONT signal to process
    kill(proc->pid, SIGCONT);
    // read 1 byte from the stdout of process
    err = read(proc->fd_stdout[0], &checkByte, 1);
    if (err <= 0) {
        perror("read");
        exit(1);
    }
}

void ProcessTerminate(Proc *proc) {
    char processOutput[100];
    int err;
    // send the 32bit simulation time
    unsigned int simulationTime = GetBigEndien(curTime);
    err = write(proc->fd_stdin[1], &simulationTime, sizeof(simulationTime));
    if (err <= 0) {
        perror("write");
        exit(1);
    }
    // Send a SIGTERM signal to process
    kill(proc->pid, SIGTERM);
    // Read a 64 byte string from the standard output of process
    err = read(proc->fd_stdout[0], processOutput, 64);
    if (err <= 0) {
        perror("read");
        exit(1);
    }
    processOutput[64] = '\0';
    printf("%u,", curTime);
    PrintProcessState(proc->state);
    printf("process_name=%s,", proc->name);
    printf("sha=%s\n", processOutput);
}

unsigned int GetBigEndien(unsigned int num) {
    unsigned int bigEndienNum = 0;
    unsigned char byte = 0;
    unsigned int mask = 0xff;
    for (int i = 0; i < 4; i++) {
        byte = (num >> (i * 8)) & mask;
        bigEndienNum |= (byte << (24 - i * 8));
    }
    return bigEndienNum;
}

void PerformanceStatistics() {
    printf("Turnaround time %d\n", (int)(totalTurnaroundTime *1.0 / totalProcess - 1e-6 + 1.0));
    printf("Time overhead %.2f %.2f\n", maxOverhead, totalOverhead / totalProcess);
    printf("Makespan %d\n", curTime);
}