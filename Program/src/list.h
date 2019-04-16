#ifndef MY_LIST_H
#define MY_LIST_H


#include <stdio.h>
#include <stdlib.h>
#include "tcb.h"

typedef struct threadControlBlock tcb;

typedef struct node
{
  tcb* thread;
  struct node* next;
}node;

typedef node* list;

void enqueue(list*, tcb*);
tcb* dequeue(list*);
void l_insert(list*, tcb*);
tcb* l_remove(list*);
tcb* thread_search(my_pthread_t, list * allThreads);
void initializeList(list*, size_t);

#endif