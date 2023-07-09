#include "allocation.h"


/// @brief I store the free memory in a linked list, and the allocated memory in another linked list.
/// @brief The free memory list is sorted by the address of the memory. It's easy to merge the memory and find the best fit memory.
/// @brief The allocated memory list isn't sorted.
Mem* freeListHead;
Mem* freeListTail;
Mem* allocatedListHead;
Mem* allocatedListTail;

Mem* MemCreate(int address, int size) {
    Mem* mem = (Mem*)malloc(sizeof(Mem));
    mem->address = address;
    mem->size = size;
    mem->next = NULL;
    mem->prev = NULL;
    return mem;
}

void AllocationCreate() {
    freeListHead = MemCreate(0, MEMORY_SIZE);
    freeListTail = freeListHead;
    allocatedListHead = NULL;
    allocatedListTail = NULL;
}

void AllocationDestroy() {
    Mem* mem = freeListHead;
    while (mem != NULL) {
        Mem* next = mem->next;
        free(mem);
        mem = next;
    }
    mem = allocatedListHead;
    while (mem != NULL) {
        Mem* next = mem->next;
        free(mem);
        mem = next;
    }
}

void AllocatedListInsert(Mem* mem) {
    if (allocatedListHead == NULL) {    // empty list
        allocatedListHead = mem;
        allocatedListTail = mem;
    } else {    // non-empty list
        allocatedListTail->next = mem;
        mem->prev = allocatedListTail;
        allocatedListTail = mem;
    }
}

Mem* AllocatedListRemove(int addr) {
    Mem* cur = allocatedListHead;
    while (cur != NULL) {
        if (cur->address == addr) {
            Mem* prev = cur->prev;
            if (prev == NULL) { // head
                allocatedListHead = cur->next;
                if (allocatedListHead != NULL) {
                    allocatedListHead->prev = NULL;
                } else {    // empty list
                    allocatedListTail = allocatedListHead;
                }
            } else {    // body or tail
                prev->next = cur->next;
                if (cur->next != NULL) {
                    cur->next->prev = prev;
                } else {    // tail
                    allocatedListTail = prev;
                }
            }
            cur->prev = NULL;
            cur->next = NULL;
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int AllocationMalloc(int size) {
    Mem* cur = freeListHead;
    Mem* mem = freeListHead;
    // no size can be larger than 2048
    int fit_size = MEMORY_SIZE + 1;
    while (cur != NULL) {
        if (cur->size >= size && cur->size < fit_size) {
            fit_size = cur->size;
            mem = cur;
        }
        cur = cur->next;
    }
    if (mem == NULL) {
        return -1;
    }
    if (mem->size == size) {    // remove from free list
        Mem* prev = mem->prev;
        if (prev == NULL) {  // head
            freeListHead = mem->next;
            if (freeListHead != NULL) {
                freeListHead->prev = NULL;
            } else {    // empty list
                freeListTail = freeListHead;
            }
        } else {    // body or tail
            prev->next = mem->next;
            if (mem->next != NULL) {
                mem->next->prev = prev;
            } else {    // tail
                freeListTail = prev;
            }
        }
        // insert into allocated list
        AllocatedListInsert(mem);
        return mem->address;
    } else {
        // update the address and size of the free list, insert the allocated memory into the allocated list
        Mem* newMem = MemCreate(mem->address, size);
        // update the address and size of mem in the free list
        mem->address += size;
        mem->size -= size;
        AllocatedListInsert(newMem);
        return newMem->address;
    }
}

void AllocationMerge(Mem* mem) {
    // merge with prev
    if (mem->prev != NULL && mem->prev->address + mem->prev->size == mem->address) {
        Mem* prev = mem->prev;
        mem->prev->size += mem->size;
        mem->prev->next = mem->next;
        if (mem->next != NULL) {
            mem->next->prev = mem->prev;
        } else {    // tail
            freeListTail = mem->prev;
        }
        free(mem);
        mem = prev;
    }
    // merge with next
    if (mem->next != NULL && mem->address + mem->size == mem->next->address) {
        Mem* next = mem->next;
        mem->size += mem->next->size;
        mem->next = mem->next->next;
        if (mem->next != NULL) {
            mem->next->prev = mem;
        } else {    // tail
            freeListTail = mem;
        }
        free(next);
    }
}

void AllocationFree(int addr) {
    Mem* mem = AllocatedListRemove(addr);
    if (mem == NULL) {
        return;
    }
    if (freeListHead == NULL) {    // empty list
        freeListHead = mem;
        freeListTail = mem;
    } else {    // non-empty list
        Mem* cur = freeListHead;
        while (cur != NULL) {
            if (cur->address > mem->address) {
                Mem* prev = cur->prev;
                if (prev == NULL) { // head
                    freeListHead = mem;
                    mem->next = cur;
                    cur->prev = mem;
                } else {    // body or tail
                    prev->next = mem;
                    mem->prev = prev;
                    mem->next = cur;
                    cur->prev = mem;
                }
                AllocationMerge(mem);
                return;
            }
            cur = cur->next;
        }
        // tail
        freeListTail->next = mem;
        mem->prev = freeListTail;
        freeListTail = mem;
        AllocationMerge(mem);
    }
}

void AllocationShow() {
    printf("freelist:\n");
    Mem* cur = freeListHead;
    while (cur != NULL) {
        printf("address: %d, size: %d\n", cur->address, cur->size);
        cur = cur->next;
    }
    printf("\nallocatedlist:\n");
    cur = allocatedListHead;
    while (cur != NULL) {
        printf("address: %d, size: %d\n", cur->address, cur->size);
        cur = cur->next;
    }
    printf("\n");
}
