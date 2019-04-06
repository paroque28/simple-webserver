#ifndef TCB_H
#define TCB_H

#include <ucontext.h>

typedef struct list list;

typedef unsigned long int my_pthread_t;
#define pthread_t my_pthread_t

//context
#define STACK_S 8192

#define MUTEX_WAIT 5
#define MAX_SIZE 200
#define INTERVAL 2000

/* mutex struct definition */
typedef struct my_pthread_mutex_t
{
  int locked;
  int available;
  int holder;
  int initialized;
  list* queue;

} my_pthread_mutex_t;

//Struct to store thread attributes
typedef struct threadControlBlock
{
  my_pthread_t tid;
  unsigned int priority;
  // Status
  //0 = ready to run, 1 = yielding, 2 = waiting, 3 = exiting, 4 = joining, 5 = waiting for mutex lock
  int status;
  void* jVal;
  void* retVal;
  ucontext_t *context;
  struct list* joinQueue;
} tcb;


#endif