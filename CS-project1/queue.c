#include "queue.h"

Node* NodeCreate(Proc *proc) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->proc = proc;
    node->prev = NULL;
    node->next = NULL;
    return node;
}
void NodeDestroy(Node *node) {
    free(node->proc);
    free(node);
}
Queue* QueueCreate() {
    Queue *queue = (Queue*)malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}
void QueueDestroy(Queue *queue) {
    Node *node = queue->head;
    while (node != NULL) {
        Node *next = node->next;
        NodeDestroy(node);
        node = next;
    }
    free(queue);
}
void QueueEnqueue(Queue *queue, Proc *proc) {
    Node* node = NodeCreate(proc);
    if (queue->head == NULL) {
        queue->head = node;
    } else {
        queue->tail->next = node;
        node->prev = queue->tail;
    }
    queue->tail = node;
    queue->size++;
}
Proc* QueueDequeue(Queue *queue) {
    if (queue->head == NULL) {
        return NULL;
    }
    Node *node = queue->head;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    } else {
        queue->head->prev = NULL;
    }
    queue->size--;
    Proc *proc = node->proc;
    free(node);
    return proc;
}
Proc* QueuePeek(Queue *queue) {
    if (queue->head == NULL) {
        return NULL;
    }
    return queue->head->proc;
}
Node* QueueIter(Queue *queue) {
    if (queue->head == NULL) {
        return NULL;
    }
    return queue->head;
}
Node* QueueIterNext(Node *node) {
    if (node == NULL) {
        return NULL;
    }
    return node->next;
}
Node* QueueIterRemove(Queue *queue, Node *node) {
    if (node == NULL) {
        return NULL;
    }
    if (node->prev == NULL) {   // head
        queue->head = node->next;
        if (queue->head == NULL) {
            queue->tail = NULL;
        } else {
            queue->head->prev = NULL;
        }
    } else if (node->next == NULL) {    // tail
        node->prev->next = NULL;
        queue->tail = node->prev;
    } else {    // body
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    Node *next = node->next;
    free(node);
    queue->size--;
    return next;
}
bool QueuePriority(Node *node1, Node *node2) {
    if (node1->proc->service_time < node2->proc->service_time) {
        return true;
    } else if (node1->proc->service_time == node2->proc->service_time) {
        if (node1->proc->time_arrived < node2->proc->time_arrived) {
            return true;
        } else if (node1->proc->time_arrived == node2->proc->time_arrived) {
            if (strcmp(node1->proc->name, node2->proc->name) < 0) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
}
void QueuePriorityEnqueue(Queue *queue, Proc *proc) {
    Node* node = NodeCreate(proc);
    if (queue->head == NULL) {
        queue->head = node;
        queue->tail = node;
    } else {
        Node *cur = queue->head;
        while (cur != NULL && QueuePriority(cur, node)) {
            cur = cur->next;
        }
        if (cur == NULL) {
            queue->tail->next = node;
            node->prev = queue->tail;
            queue->tail = node;
        } else if (cur->prev == NULL) {
            queue->head = node;
            node->next = cur;
            cur->prev = node;
        } else {
            cur->prev->next = node;
            node->prev = cur->prev;
            node->next = cur;
            cur->prev = node;
        }
    }
    queue->size++;
}
bool QueueEmpty(Queue *queue) {
    return queue->size == 0;
}
void QueueShow(Queue *queue) {
    Node *node = queue->head;
    while (node != NULL) {
        printf("Time arrived: %u Name: %s Service time: %u Memory required: %u\n", 
                node->proc->time_arrived, 
                node->proc->name, 
                node->proc->service_time, 
                node->proc->memory_required);
        node = node->next;
    }
}


