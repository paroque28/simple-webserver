#ifndef TCB_H
#define TCB_H

#include <ucontext.h>
#include <sys/queue.h>
#include "list.h"

//List typedef circular dependency
typedef struct head_s head_t;

// Initialization
struct threadControlBlock;
typedef struct threadControlBlock tcb;

// My pthread structs
typedef unsigned long int my_pthread_t;
typedef void my_pthread_mutexattr_t;

//pthread interface
#define pthread_t my_pthread_t
#define pthread_attr_t my_pthread_attr_t
#define pthread_mutex_t my_pthread_mutex_t

//context
#define STACK_S 8192
#define MAX_SIZE 200

// Pthread attr definition
typedef struct my_pthread_attr
{ 
  // Lottery
  unsigned int tickets;

  //Real-Time
  unsigned int period; //Number of quantums between execution
  unsigned int duration; // Number of quantums per frame
  
} my_pthread_attr_t;

// mutex struct definition 
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

  // Lottery
  unsigned int tickets;
  
  //SelfishRR
  unsigned long selfishScore;

  //Real-Time
  unsigned long quantumsRun;
  unsigned long long lastRun;
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