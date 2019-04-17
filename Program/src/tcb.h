#ifndef TCB_H
#define TCB_H

#include <ucontext.h>
#include <sys/queue.h>
#include "list.h"

typedef struct head_s head_t;
// Initialization
struct threadControlBlock;
typedef struct threadControlBlock tcb;


typedef unsigned long int my_pthread_t;
typedef void my_pthread_attr_t;
typedef void my_pthread_mutexattr_t;

#define pthread_t my_pthread_t
#define pthread_attr_t my_pthread_attr_t
//context
#define STACK_S 8192

#define MUTEX_WAIT 5
#define MAX_SIZE 200


/* mutex struct definition */
typedef struct my_pthread_mutex_t
{
  int locked;
  int holder;
  int initialized;
  head_t* queue;

} my_pthread_mutex_t;

//Struct to store thread attributes
typedef struct threadControlBlock
{
  my_pthread_t tid;
  //Real-Time
  unsigned long quantumsRun;
  unsigned long lastRun;
  unsigned int period; //Number of quantums between execution
  unsigned int duration; // Number of quantums per frame

  // Status
  //0 = ready to run, 1 = yielding, 2 = waiting, 3 = exiting, 4 = joining, 5 = waiting for mutex lock
  int status;
  void* jVal;
  void* retVal;
  ucontext_t* context;
  head_t* joinQueue;
} tcb;


#endif