#include "list.h"

//add to queue
void enqueue(list* q, tcb* insert)
{
  list queue = *q;

  if(queue == NULL)
  {
    queue = (list)malloc(sizeof(list));
    queue->thread = insert;
    queue->next = queue;
    *q = queue;
    return;
  }

  list front = queue->next;
  queue->next = (list)malloc(sizeof(list));
  queue->next->thread = insert;
  queue->next->next = front;

  queue = queue->next;
  *q = queue;
  return;
}

//remove from queue
tcb* dequeue(list* q)
{
  list queue = *q;
  if(queue == NULL)
  {
    return NULL;
  }
  //queue is the last element in a queue at level i
  //first get the thread control block to be returned
  list front = queue->next;
  tcb *tgt = queue->next->thread;
  //check if there is only one element left in the queue
  //and assign null/free appropriately
  if(queue->next == queue)
  { 
    queue = NULL;
  }
  else
  {
    queue->next = front->next;
  }
  free(front);

  
  if(tgt == NULL)
  {printf("WE HAVE A PROBLEM IN DEQUEUE\n");}

  *q = queue;
  return tgt;
}

//insert to list
void l_insert(list* q, tcb* jThread) //Non-circular Linked List
{
  list queue = *q;

  if(queue == NULL)
  {
    queue = (list)malloc(sizeof(list));
    queue->thread = jThread;
    queue->next = NULL;
    *q = queue;
    return;
  }

  list newNode = (list)malloc(sizeof(list));
  newNode->thread = jThread;

  //append to front of LL
  newNode->next = queue;
  
  queue = newNode;
  *q = queue;
  return;
}

//remove from list
tcb* l_remove(list* q)
{
  list queue = *q;

  if(queue == NULL)
  {
    return NULL;
  }

  list temp = queue;
  tcb *ret = queue->thread;
  queue = queue->next;
  free(temp);
  *q = queue;
  return ret;
}


//Search table for a tcb given a uint
tcb* thread_search(my_pthread_t tid, list * allThreads)
{
  int key = tid % MAX_SIZE;
  tcb *ret = NULL;

  list temp = allThreads[key];
  while(allThreads[key] != NULL)
  {
    if(allThreads[key]->thread->tid == tid)
    {
      ret = allThreads[key]->thread;
      break;
    }
    allThreads[key] = allThreads[key]->next;
  }

  allThreads[key] = temp;

  return ret;
}



void initializeList(list* list, size_t size) 
{
  int i;
  for(i = 0; i < size; i++) 
  {
    list[i] = NULL;
  }
  
}