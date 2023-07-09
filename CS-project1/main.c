#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "queue.h"
#include "scheduler.h"
#include "allocation.h"

#define IMPLEMENTS_REAL_PROCESS

int main(int argc, char *argv[]) {
    int opt;
    char* filename = NULL;
    int scheduler_mode = 0;
    int memory_strategy_mode = 0;
    int quantum = 1;
    // parse command line arguments
    while ((opt = getopt(argc, argv, "f:s:m:q:")) != -1) {
        switch (opt) {
            case 'f':
                filename = optarg;
                break;
            case 's':
                if (strcmp(optarg, "SJF") == 0) {
                    scheduler_mode = SJF;
                } else if (strcmp(optarg, "RR") == 0) {
                    scheduler_mode = RR;
                } else {
                    fprintf(stderr, "Usage: %s -f <filename> -s (SJF | RR) -m (infinite | best-fit) -q (1 | 2 | 3)\n", argv[0]);
                    exit(1);
                }
                break;
            case 'm':
                if (strcmp(optarg, "infinite") == 0) {
                    memory_strategy_mode = INFINITE;
                } else if (strcmp(optarg, "best-fit") == 0) {
                    memory_strategy_mode = BEST_FIT;
                } else {
                    fprintf(stderr, "Usage: %s -f <filename> -s (SJF | RR) -m (infinite | best-fit) -q (1 | 2 | 3)\n", argv[0]);
                    exit(1);
                }
                break;
            case 'q':
                quantum = atoi(optarg);
                if (quantum < 1 || quantum > 3) {
                    fprintf(stderr, "Usage: %s -f <filename> -s (SJF | RR) -m (infinite | best-fit) -q (1 | 2 | 3)\n", argv[0]);
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Usage: %s -f <filename> -s (SJF | RR) -m (infinite | best-fit) -q (1 | 2 | 3)\n", argv[0]);
                exit(1);
        }
    }

    SchedulerCreate(filename);
    AllocationCreate();
    SchedulerRun(scheduler_mode, memory_strategy_mode, quantum);
    PerformanceStatistics();
    SchedulerDestroy();
    AllocationDestroy();

    return 0;
}