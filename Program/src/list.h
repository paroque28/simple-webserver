#ifndef MY_LIST_H
#define MY_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "tcb.h"

typedef struct threadControlBlock tcb;

//Queue
typedef struct node
{
    tcb* thread;
    TAILQ_ENTRY(node) nodes;  /* Tail queue. */
} node_t;

// This typedef creates a head_t that makes it easy for us to pass pointers to
// head_t without the compiler complaining.
typedef TAILQ_HEAD(head_s, node) head_t;

//Methods

void initQueue(head_t* queue);
void enqueue(head_t* queue, tcb* thread);
void insert(head_t* queue, tcb* thread);

tcb* dequeue(head_t* queue);


void printQueue(head_t* queue);

#endif