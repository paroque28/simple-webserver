#include "my_pthread.h"

#define STACK_S 8192 //8kB stack frames
#define READY 0
#define YIELD 1
#define WAIT 2
#define EXIT 3
#define JOIN 4
#define MUTEX_WAIT 5
#define MAX_SIZE 15
#define INTERVAL 20000

tcb *currentThread, *prevThread;
list *runningQueue[MAX_SIZE];
list *allThreads[MAX_SIZE];
ucontext_t cleanup;
sigset_t signal_set;
mySig sig;

struct itimerval timer, currentTime;

int mainContextInitialized;
int timeElapsed;
int threadCount;
int operationInProgress;

void scheduler(int signum)
{
  printf("Scheduler in!\n");
  if(operationInProgress)
  {
    printf("Operation in Progress!\n");
    return;
  }
  //Record remaining time
  getitimer(ITIMER_VIRTUAL, &currentTime);
  printf("Get Timer\n");

  //AT THIS POINT THE CURRENT THREAD IS NOT IN THE SCHEDULER (running queue, but it's always in allthreads)
  //once the timer finishes, the value of it_value.tv_usec will reset to the interval time (note this was when we set it_interval only)
  //printf("\n[Thread %d] Signaled from %d, time left %i\n", currentThread->tid,currentThread->tid, (int)currentTime.it_value.tv_usec);

  //disable timer if still activehttps://www.google.com/search?q=complete+v
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_VIRTUAL, &timer, NULL);

  if(signum != SIGVTALRM)
  {
    /*TODO: PANIC*/
    //printf("[Thread %d] Signal Received: %d.\nExiting...\n", currentThread->tid, signum);
    exit(signum);
  }


  //Time elapsed = difference between max interval size and time remaining in timer
  //if the time splice runs to completion the else body goes,
  //else the if body goes, and the actual amount of time that passed is added to timeelapsed
  int timeSpent = (int)currentTime.it_value.tv_usec;
  int expectedInterval = INTERVAL * (currentThread->priority + 1);
  //printf("timeSpent: %i, expectedInterva%i\n", timeSpent, expectedInterval);
  if(timeSpent < 0 || timeSpent > expectedInterval)
  {
    timeSpent = 0;
  }
  else
  {
    timeSpent = expectedInterval - timeSpent;
  }

  
  timeElapsed += timeSpent;
  //printf("total time spend so far before maintenance cycle %i and the amount of time spent just now %i\n", timeElapsed, timeSpent);
  //printf("[Thread %d] Total time: %d from time remaining: %d out of %d\n", currentThread->tid, timeElapsed, (int)currentTime.it_value.tv_usec, INTERVAL * (currentThread->priority + 1));

  //check for maintenance cycle
  if(timeElapsed >= 10000000)
  {
    //printf("\n[Thread %d] MAINTENANCE TRIGGERED\n\n",currentThread->tid);
    maintenance();

    //reset counter
    timeElapsed = 0;
  }

  prevThread = currentThread;
  
  int i;

  switch(currentThread->status)
  {
    case READY: //READY signifies that the current thread is in the running queue

      if(currentThread->priority < MAX_SIZE - 1)
      {
	currentThread->priority++;
      }

      //put back the thread that just finished back into the running queue
      enqueue(&runningQueue[currentThread->priority], currentThread);

      currentThread = NULL;

      for(i = 0; i < MAX_SIZE; i++) 
      {
        if (runningQueue[i] != NULL)
        { 
          //getting a new thread to run
          currentThread = dequeue(&runningQueue[i]);
	  break;
        }
	else
	{
	}
      }

      if(currentThread == NULL)
      {
        currentThread = prevThread;
      }

      break;
   
    case YIELD: //YIELD signifies pthread yield was called; don't update priority

      currentThread = NULL;

      for (i = 0; i < MAX_SIZE; i++) 
      {
        if (runningQueue[i] != NULL)
        { 
          currentThread = dequeue(&runningQueue[i]);
	  break;
        }
      }
      
      if(currentThread != NULL)
      {
	//later consider enqueuing it to the waiting queue instead
	enqueue(&runningQueue[prevThread->priority], prevThread);
      }
      else
      {
	currentThread = prevThread;
      }

      break;

    case WAIT:
      //When would something go to waiting queue?
      //A: In the case of blocking I/O, how do we detect this? Sockets
      //GG NOT OUR PROBLEM ANYMORE
      //enqueue(&waitingQueue, currentThread);
      
      break;

    case EXIT:

      currentThread = NULL;

      for (i = 0; i < MAX_SIZE; i++) 
      {
        if (runningQueue[i] != NULL)
        {
          currentThread = dequeue(&runningQueue[i]);
	  break;
        }
      }

      if(currentThread == NULL)
      {
	      printf("No other threads found. Exiting\n");
        printf("Scheduler Out!\n");
        return;
      }
      //free the thread control block and ucontext
      free(prevThread->context->uc_stack.ss_sp);
      free(prevThread->context);
      free(prevThread);

      currentThread->status = READY;

      //printf("Switching to: TID %d Priority %d\n", currentThread->tid, currentThread->priority);

      //reset timer
      timer.it_value.tv_sec = 0;
      timer.it_value.tv_usec = INTERVAL * (currentThread->priority + 1);
      timer.it_interval.tv_sec = 0;
      timer.it_interval.tv_usec = 0;
      int ret = setitimer(ITIMER_VIRTUAL, &timer, NULL);
      if (ret < 0)
      {
        //printf("Timer Reset Failed. Exiting...\n");
        exit(0);
      }
      setcontext(currentThread->context);

      break;

    case JOIN: //JOIN corresponds with a call to pthread_join

      currentThread = NULL;
      //notice how we don't enqueue the thread that just finished back into the running queue
      //we just go straight to getting another thread
      for (i = 0; i < MAX_SIZE; i++) 
      {
        if (runningQueue[i] != NULL)
        { 
          currentThread = dequeue(&runningQueue[i]);
	  break;
        }
      }

      if(currentThread == NULL)
      {
	/*TODO: WE'VE GOT A PROBLEM*/
	exit(EXIT_FAILURE);
      }
      
      break;
      
    case MUTEX_WAIT: //MUTEX_WAIT corresponds with a thread waiting for a mutex lock

      //Don't add current to queue: already in mutex queue
      currentThread = NULL;

      for (i = 0; i < MAX_SIZE; i++) 
      {
        if (runningQueue[i] != NULL)
        { 
          currentThread = dequeue(&runningQueue[i]);
	  break;
        }
      }

      if(currentThread == NULL)
      {
        /*TODO: OH SHIT DEADLOCK*/
        //printf("DEADLOCK DETECTED\n");
	exit(EXIT_FAILURE);
      }

      break;

    default:
      //printf("Thread Status Error: %d\n", currentThread->status);
      exit(-1);
      break;
  }
	
  currentThread->status = READY;

  //reset timer to 25ms times thread priority
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = INTERVAL * (currentThread->priority + 1) ;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  int ret = setitimer(ITIMER_VIRTUAL, &timer, NULL);

  if (ret < 0)
  {
     //printf("Timer Reset Failure. Exiting...\n");
     exit(0);
  }

  //printf("Switching to: TID %d Priority %d\n", currentThread->tid, currentThread->priority);
  //Switch to new context
  if(prevThread->tid == currentThread->tid)  
  {/*Assume switching to same context is bad. So don't do it.*/}
  else
  {swapcontext(prevThread->context, currentThread->context);}
  printf("Scheduler Out!\n");
  return;

}

//thread priority boosting
void maintenance()
{
  int i;
  tcb *tgt;


  //template for priority inversion
  for(i = 1; i < MAX_SIZE; i++)
  {
    while(runningQueue[i] != NULL)
    {
      tgt = dequeue(&runningQueue[i]);
      tgt->priority = 0;
      enqueue(&runningQueue[0], tgt);
    }
  }

  return;
}

void garbage_collection()
{
  operationInProgress = 1;
  currentThread->status = EXIT;
  
  //if no threads exit
  if(!mainContextInitialized) exit(EXIT_SUCCESS);

  tcb *jThread = NULL; //any threads waiting on the one being garbage collected

  //dequeue all threads waiting on this one to finish
  while(currentThread->joinQueue != NULL)
  {
    jThread = l_remove(&currentThread->joinQueue);
    jThread->retVal = currentThread->jVal;
    enqueue(&runningQueue[jThread->priority], jThread);
  }

  //free stored node in allThreads
  int key = currentThread->tid % MAX_SIZE;
  if(allThreads[key]->thread->tid == currentThread->tid)
  {
    list *removal = allThreads[key];
    allThreads[key] = allThreads[key]->next;
    free(removal); 
  }

  else
  {
    list *temp = allThreads[key];
    while(allThreads[key]->next != NULL)
    {
      if(allThreads[key]->next->thread->tid == currentThread->tid)
      {
	list *removal = allThreads[key]->next;
	allThreads[key]->next = removal->next;
	free(removal);
        break;
      }
      allThreads[key] = allThreads[key]->next;
    }

    allThreads[key] = temp;
  }

  operationInProgress = 0;

  raise(SIGVTALRM);
}

//add to queue
void enqueue(list** q, tcb* insert)
{
  list *queue = *q;

  if(queue == NULL)
  {
    queue = (list*)malloc(sizeof(list));
    queue->thread = insert;
    queue->next = queue;
    *q = queue;
    return;
  }

  list *front = queue->next;
  queue->next = (list*)malloc(sizeof(list));
  queue->next->thread = insert;
  queue->next->next = front;

  queue = queue->next;
  *q = queue;
  return;
}

//remove from queue
tcb* dequeue(list** q)
{
  list *queue = *q;
  if(queue == NULL)
  {
    return NULL;
  }
  //queue is the last element in a queue at level i
  //first get the thread control block to be returned
  list *front = queue->next;
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
void l_insert(list** q, tcb* jThread) //Non-circular Linked List
{
  list *queue = *q;

  if(queue == NULL)
  {
    queue = (list*)malloc(sizeof(list));
    queue->thread = jThread;
    queue->next = NULL;
    *q = queue;
    return;
  }

  list *newNode = (list*)malloc(sizeof(list));
  newNode->thread = jThread;

  //append to front of LL
  newNode->next = queue;
  
  queue = newNode;
  *q = queue;
  return;
}

//remove from list
tcb* l_remove(list** q)
{
  list *queue = *q;

  if(queue == NULL)
  {
    return NULL;
  }

  list *temp = queue;
  tcb *ret = queue->thread;
  queue = queue->next;
  free(temp);
  *q = queue;
  return ret;
}


//Search table for a tcb given a uint
tcb* thread_search(my_pthread_t tid)
{
  int key = tid % MAX_SIZE;
  tcb *ret = NULL;

  list *temp = allThreads[key];
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

void initializeMainContext()
{
  printf("Initializing main context!\n");
  tcb *mainThread = (tcb*)malloc(sizeof(tcb));
  ucontext_t *mText = (ucontext_t*)malloc(sizeof(ucontext_t));
  getcontext(mText);
  mText->uc_link = &cleanup;

  mainThread->context = mText;
  mainThread->tid = 0;
  mainThread->priority = 0;
  mainThread->joinQueue = NULL;
  mainThread->jVal = NULL;
  mainThread->retVal = NULL;
  mainThread->status = READY;

  mainContextInitialized = 1;

  l_insert(&allThreads[0], mainThread);

  currentThread = mainThread;
  printf("Main context initialized!\n");
}

void initializeGarbageContext()
{
  //Set handler of timer
  signal(SIGVTALRM, scheduler);
  initializeQueues(runningQueue); //set everything to NULL
    
  //Initialize garbage collector
  getcontext(&cleanup);
  cleanup.uc_link = NULL;
  cleanup.uc_stack.ss_sp = malloc(STACK_S);
  cleanup.uc_stack.ss_size = STACK_S;
  cleanup.uc_stack.ss_flags = 0;
  makecontext(&cleanup, (void*)&garbage_collection, 0);

  //set thread count
  threadCount = 1;
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg)
{

  if(!mainContextInitialized)
  {
    initializeGarbageContext();
  }

  operationInProgress = 1;

  //Create a thread context to add to scheduler
  ucontext_t* task = (ucontext_t*)malloc(sizeof(ucontext_t));
  getcontext(task);
  task->uc_link = &cleanup;
  task->uc_stack.ss_sp = malloc(STACK_S);
  task->uc_stack.ss_size = STACK_S;
  task->uc_stack.ss_flags = 0;
  makecontext(task, (void*)function, 1, arg);
  //printf("Context created!\n");
  tcb *newThread = (tcb*)malloc(sizeof(tcb));
  newThread->context = task;
  newThread->tid = threadCount;
  newThread->priority = 0;
  newThread->joinQueue = NULL;
  newThread->jVal = NULL;
  newThread->retVal = NULL;
  newThread->status = READY;

  *thread = threadCount;
  threadCount++;
  //printf("Thread created: TID %ld\n", newThread->tid);
  enqueue(&runningQueue[0], newThread);
  int key = newThread->tid % MAX_SIZE;

  l_insert(&allThreads[key], newThread);

  operationInProgress = 0;

  //store main context
  if (!mainContextInitialized)
  {
    //printf("New context: TID %ld\n", newThread->tid);
    initializeMainContext();
    //raise(SIGVTALRM);
  }
  printf("New thread created: TID %ld\n", newThread->tid);
  
  raise(SIGVTALRM);
  return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield()
{
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  //return to signal handler/scheduler
  currentThread->status = YIELD;
  return raise(SIGVTALRM);
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr)
{
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  //call garbage collection
  currentThread->jVal = value_ptr;
  setcontext(&cleanup);
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr)
{
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  operationInProgress = 1;
  //make sure thread can't wait on self
  if(thread == currentThread->tid)
  {return -1;}

  tcb *tgt = thread_search(thread);
  
  if(tgt == NULL)
  {
    return -1;
  }
  
  //Priority Inversion Case
  tgt->priority = 0;

  l_insert(&tgt->joinQueue, currentThread);

  currentThread->status = JOIN;

  operationInProgress = 0;
  raise(SIGVTALRM);

  if(value_ptr == NULL)
  {return 0;}

  *value_ptr = currentThread->retVal;

  return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
  //TODO: Undefined behavior if init called on locked mutex
  operationInProgress = 1;
  my_pthread_mutex_t m = *mutex;
  
  m.available = 1;
  m.locked = 0;
  m.holder = -1; //holder represents the tid of the thread that is currently holding the mutex
  m.queue = NULL;

  *mutex = m;
  operationInProgress = 0;
  return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex)
{
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  operationInProgress = 1;

  //FOR NOW ASSUME MUTEX WAS INITIALIZED
  if(!mutex->available)
  {return -1;}

  while(__atomic_test_and_set((volatile void *)&mutex->locked,__ATOMIC_RELAXED))
  {
    //the reason why we reset operationInProgress to one here is that when coming back
    //from a swapcontext, operationInProgress may be zero and we can't let the operations
    //in the loop be interrupted
    operationInProgress = 1;
    enqueue(&mutex->queue, currentThread);
    currentThread->status = MUTEX_WAIT;
    //we need to set operationInProgress to zero before going to scheduler
    operationInProgress = 0;
    raise(SIGVTALRM);
  }

  if(!mutex->available)
  {
    mutex->locked = 0;
    return -1;
  }

  //Priority Inversion Case
  currentThread->priority = 0;
  mutex->holder = currentThread->tid;
  
  operationInProgress = 0;
  return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  //NOTE: handle errors: unlocking an open mutex, unlocking mutex not owned by calling thread, or accessing unitialized mutex

  operationInProgress = 1;

  //ASSUMING mutex->available will be initialized to 0 by default without calling init
  //available in this case means that mutex has been initialized or destroyed (state variable)
  if(!mutex->available || !mutex->locked || mutex->holder != currentThread->tid)
  {return -1;}

  mutex->locked = 0;
  mutex->holder = -1;

  tcb* muThread = dequeue(&mutex->queue);
  
  if(muThread != NULL)
  {
    //Priority Inversion Case
    muThread->priority = 0;
    enqueue(&runningQueue[0], muThread);
  }

  operationInProgress = 0;

  return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex)
{
  operationInProgress = 1;
  my_pthread_mutex_t m = *mutex;
  //prevent threads from accessing while mutex is in destruction process
  m.available = 0;
  operationInProgress = 0;

  //if mutex still locked, wait for thread to release lock
  while(m.locked)
  {raise(SIGVTALRM);}

  //TODO: Undefined behavior if mutex queue isn't empty when being destroyed
  tcb *muThread;
  while(m.queue != NULL)
  {
    muThread = dequeue(&m.queue);
    enqueue(&runningQueue[muThread->priority], muThread);
  }

  *mutex = m;
  return 0;
};

void initializeQueues(list** runQ) 
{
  int i;
  for(i = 0; i < MAX_SIZE; i++) 
  {
    runningQueue[i] = NULL;
    allThreads[i] = NULL;
  }
  
}