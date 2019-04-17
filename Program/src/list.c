#include "list.h"

void enqueue(head_t* queue, tcb* thread){
  node_t * threadNode = malloc(sizeof(node_t));
  threadNode->thread = thread;
  TAILQ_INSERT_HEAD(queue, threadNode, nodes);
}
void insert(head_t* queue, tcb* thread){
  node_t * threadNode = malloc(sizeof(node_t));
  threadNode->thread = thread;
  TAILQ_INSERT_TAIL(queue, threadNode, nodes);
}

tcb* dequeue(head_t* queue){
  node_t * threadNode = TAILQ_FIRST(queue);
  tcb* thread = threadNode->thread;
  TAILQ_REMOVE(queue, threadNode, nodes);
  free(threadNode);
  return thread;
}