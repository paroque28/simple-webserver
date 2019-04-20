#include "list.h"

void initQueue(head_t* queue){
  TAILQ_INIT(queue);
}

void enqueue(head_t* queue, tcb* thread){
  node_t * threadNode = malloc(sizeof(node_t));
  threadNode->thread = thread;
  TAILQ_INSERT_TAIL(queue, threadNode, nodes);
}
void insert(head_t* queue, tcb* thread){
  node_t * threadNode = malloc(sizeof(node_t));
  threadNode->thread = thread;
  TAILQ_INSERT_HEAD(queue, threadNode, nodes);
}

tcb* dequeue(head_t* queue){
  node_t * threadNode = TAILQ_FIRST(queue);
  if (threadNode == NULL) return NULL;
  tcb* thread = threadNode->thread;
  TAILQ_REMOVE(queue, threadNode, nodes);
  free(threadNode);
  return thread;
}

int removeFromQueue(head_t* queue, tcb* thread){
  struct node * e = NULL;
  int res = 1;
  TAILQ_FOREACH(e, queue, nodes)
  {
      if(e->thread == thread){
        TAILQ_REMOVE(queue, e, nodes);
        free(e);
        res = 0;
        break;
      }
  }
  return res;
}


void printQueue(head_t* queue){

  struct node * e = NULL;
  printf ("\nHEAD=> ");
  TAILQ_FOREACH(e, queue, nodes)
  {
      printf("\tThread %ld\n", e->thread->tid);
  }
}