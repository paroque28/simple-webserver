#include "my_pthread.h"

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const my_pthread_mutexattr_t *mutexattr)
{
  
  operationInProgress = 1;
  my_pthread_mutex_t m = *mutex;
  
  m.initialized = 1;
  m.locked = 0;
  m.holder = -1; //holder represents the tid of the thread that is currently holding the mutex
  m.queue = malloc(sizeof(head_t*));;
  initQueue(m.queue);

  *mutex = m;
  operationInProgress = 0;
  //printf("Mutex initialized !\n");
  return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex)
{
  //printf("Mutex lock !\n");
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  operationInProgress = 1;

  if(!mutex->initialized)
  {return -1;}
  printf("Lock status: %d\n",mutex->locked);
  while(__atomic_test_and_set((volatile void *)&mutex->locked,__ATOMIC_RELAXED))
  {
    //printf("Mutex is locked!\n");
    operationInProgress = 1;
    enqueue(mutex->queue, currentThread);
    currentThread->status = MUTEX_WAIT;
    //we need to set notFinished to zero before going to scheduler
    operationInProgress = 0;
    //printf("RAISE Mutex is locked!\n");
    raise(SIGUSR2);
    //printf("RAISED Mutex is locked!\n");
  }

  if(!mutex->initialized)
  {
    mutex->locked = 0;
    return -1;
  }

  mutex->holder = currentThread->tid;
  
  operationInProgress = 0;
  //printf("Mutex locked !\n");
  return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
  //printf("Mutex unlock !\n");
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  
  operationInProgress = 1;

  if(!mutex->initialized || !mutex->locked || mutex->holder != currentThread->tid)
  {return -1;}

  mutex->locked = 0;
  mutex->holder = -1;

  tcb* muThread;
  

  while(!TAILQ_EMPTY(mutex->queue))
  {
    muThread = dequeue(mutex->queue);
    if(muThread != NULL) 
      enqueue(&runningQueue, muThread);
  }
  operationInProgress = 0;
  
  //printf("Mutex unlocked !\n");
  return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex)
{
  operationInProgress = 1;
  my_pthread_mutex_t m = *mutex;
  //Prevent threads from accessing while mutex is in destruction process
  m.initialized = 0;
  operationInProgress = 0;

  //If mutex still locked, wait for thread to release lock
  while(m.locked)
  {
    printf("Mutex still locked while removing!\n");
    raise(SIGUSR2);
  }

  tcb *muThread;
  while(!TAILQ_EMPTY(m.queue))
  {
    muThread = dequeue(m.queue);
    if(muThread != NULL) 
      enqueue(&runningQueue, muThread);
  }
  *mutex = m;
  return 0;
};