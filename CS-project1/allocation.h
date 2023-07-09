#ifndef ALLOCATION_H
#define ALLOCATION_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MEMORY_SIZE 2048

typedef struct Mem {
    int address;
    int size;
    struct Mem *next;
    struct Mem *prev;
} Mem;

Mem* MemCreate(int address, int size);
void AllocationCreate();
void AllocationDestroy();
void AllocatedListInsert(Mem* mem);
Mem* AllocatedListRemove(int addr);
int AllocationMalloc(int size);
void AllocationMerge(Mem* mem);
void AllocationFree(int addr);
void AllocationShow();

#endif