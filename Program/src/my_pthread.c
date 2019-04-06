#include "my_pthread.h"

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
  // printf("Scheduler in!\n");
  if(operationInProgress)
  {
    printf("Operation in Progress!\n");
    return;
  }
  //Record remaining time
  getitimer(ITIMER_VIRTUAL, &currentTime);
  // printf("Get Time\n");

  // printf("\n[Thread %ld] Signaled from %ld, time left %i\n", currentThread->tid,currentThread->tid, (int)currentTime.it_value.tv_usec);

  //Disable Timer to prevent SIGVALRM
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_VIRTUAL, &timer, NULL);

  if(signum != SIGVTALRM)
  {
    printf("[Thread %ld] Signal Received: %d.\nExiting...\n", currentThread->tid, signum);
    exit(signum);
  }


  // Time elapsed = difference between max interval size and time remaining in timer
  // if the time splice doesn't deplates the else body goes 
  // the actual amount of time that passed is added to timeelapsed
  int timeSpent = (int)currentTime.it_value.tv_usec;
  int expectedInterval = INTERVAL * (currentThread->priority + 1);
  //printf("timeSpent: %i, expectedInterval%i\n", timeSpent, expectedInterval);
  if(timeSpent < 0 || timeSpent > expectedInterval)
  {
    timeSpent = 0;
  }
  else
  {
    timeSpent = expectedInterval - timeSpent;
  }

  
  timeElapsed += timeSpent;
  //printf("total time spend so far before boost_thread_priorities cycle %i and the amount of time spent just now %i\n", timeElapsed, timeSpent);
  //printf("[Thread %ld] Total time: %d from time remaining: %d out of %d\n", currentThread->tid, timeElapsed, (int)currentTime.it_value.tv_usec, INTERVAL * (currentThread->priority + 1));

  //check for threads with great timeElapsed
  if(timeElapsed >= 10000000)
  {
    printf("\n[Thread %ld] boost TRIGGERED\n\n",currentThread->tid);
    boost_thread_priorities();
    timeElapsed = 0;
  }

  prevThread = currentThread;
  
  int i;

  switch(currentThread->status)
  {
    case READY: //READY current thread is in the running queue
      //Higher the priority
      if(currentThread->priority < MAX_SIZE - 1)  currentThread->priority++;

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
   
    case YIELD: //YIELD pthread yield was called; don't update priority

      currentThread = NULL;

      for (i = 0; i < MAX_SIZE; i++) 
      {
        if (runningQueue[i] != NULL)
        { 
          currentThread = dequeue(&runningQueue[i]);
	  break;
        }
      }
      //IF then later consider enqueuing it to the waiting queue instead
      if(currentThread != NULL)  enqueue(&runningQueue[prevThread->priority], prevThread);
      else currentThread = prevThread;
    
      break;

    case WAIT:
     
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
        return;
      }
      //free the thread control block and ucontext
      free(prevThread->context->uc_stack.ss_sp);
      free(prevThread->context);
      free(prevThread);

      currentThread->status = READY;

      //printf("-Switching to: TID %ld Priority %d\n", currentThread->tid, currentThread->priority);

      //reset timer
      timer.it_value.tv_sec = 0;
      timer.it_value.tv_usec = INTERVAL * (currentThread->priority + 1);
      timer.it_interval.tv_sec = 0;
      timer.it_interval.tv_usec = 0;
      int ret = setitimer(ITIMER_VIRTUAL, &timer, NULL);
      if (ret < 0)
      {
        printf("Timer Reset Failed. Exiting...\n");
        exit(0);
      }
      setcontext(currentThread->context);

      break;

    case JOIN: //JOIN pthread_join

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
        printf("Error currentThread is NULL");
	      exit(EXIT_FAILURE);
      }
      
      break;
      
    case MUTEX_WAIT: //MUTEX_WAIT mutex lock

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
        printf("DEADLOCK DETECTED\n");
	exit(EXIT_FAILURE);
      }

      break;

    default:
      printf("Thread Status Error: %d\n", currentThread->status);
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
     printf("Timer Reset Failure. Exiting...\n");
     exit(0);
  }

  
  //Switch to new context
  if(prevThread->tid == currentThread->tid)  
  {
      //printf("Switching to same thread: TID %ld Priority %d\n", currentThread->tid, currentThread->priority);
  }
  else
  {
    //printf("Switching to: TID %ld Priority %d\n", currentThread->tid, currentThread->priority);
    swapcontext(prevThread->context, currentThread->context);
  }
  //printf("Scheduler Out!\n");
  return;

}

//thread priority boosting
void boost_thread_priorities()
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


void initializeMainContext()
{
  //printf("Initializing main context!\n");
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
  // printf("Main context initialized!\n");
}

void initializeGarbageContext()
{
  //Set handler of timer
  signal(SIGVTALRM, scheduler);
  initializeQueues(runningQueue,allThreads,runningQueue); //set everything to NULL
    
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
  //printf("New thread created: TID %ld\n", newThread->tid);
  
  raise(SIGVTALRM);
  return 0;
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

  tcb *tgt = thread_search(thread,allThreads);
  
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

//Busy waiting
void my_sleep(unsigned long seconds){
    time_t start = time(NULL);
    while (1){
        time_t now = time(NULL);
        if(now >= start + 1 * seconds) return;
    }
}