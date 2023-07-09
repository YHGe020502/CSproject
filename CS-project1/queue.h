#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "scheduler.h"

typedef struct Proc Proc;

typedef struct Node {
    Proc *proc;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *head;
    Node *tail;
    size_t size;
} Queue;

Node* NodeCreate(Proc *proc);
void NodeDestroy(Node *node);
Queue* QueueCreate();
void QueueDestroy(Queue *queue);
void QueueEnqueue(Queue *queue, Proc *proc);
Proc* QueueDequeue(Queue *queue);
Proc* QueuePeek(Queue *queue);
Node* QueueIter(Queue *queue);
Node* QueueIterNext(Node *node);
Node* QueueIterRemove(Queue *queue, Node *node);
bool QueuePriority(Node *node1, Node *node2);
void QueuePriorityEnqueue(Queue *queue, Proc *proc);
bool QueueEmpty(Queue *queue);
void QueueShow(Queue *queue);

#endif