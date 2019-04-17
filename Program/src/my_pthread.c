#include "my_pthread.h"

void scheduler(int signum)
{
  if(operationInProgress)
  {
    printf("Operation in Progress!\n");
    return;
  }
  
  //Record remaining time
  getitimer(ITIMER_VIRTUAL, &currentTime);

  // /printf("\n[Thread %ld] Signaled from %ld, time left %i\n", currentThread->tid,currentThread->tid, (int)currentTime.it_value.tv_usec);

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
  int expectedInterval = INTERVAL;
  //printf("timeSpent: %i, expectedInterval %i\n", timeSpent, expectedInterval);
  if(timeSpent < 0 || timeSpent > expectedInterval)
  {
    timeSpent = 0;
  }
  else
  {
    timeSpent = expectedInterval - timeSpent;
  }

  
  timeElapsed += timeSpent;

  //printf("[Thread %ld] Total time: %d from time remaining: %d out of %d\n", currentThread->tid, timeElapsed, (int)currentTime.it_value.tv_usec, INTERVAL);


  prevThread = currentThread;
  
  int i;
  switch(currentThread->status)
  {
    case READY: //READY current thread is in the running queue

      //put back the thread that just finished back into the running queue

      //if(scheduler == fifo){}
      // enqueue currentThread into running queue
      enqueue(&runningQueue, currentThread);

      // Dequeue new thread from runningQueue
      currentThread =  dequeue(&runningQueue);

      break;
   
    case YIELD: //YIELD pthread yield was called; don't update priority

      // Dequeue new thread from runningQueue
      currentThread =dequeue(&runningQueue);

      //IF then later consider enqueuing it to the waiting queue instead
      if(currentThread != NULL){
        enqueue(&runningQueue, prevThread);
      }
      else currentThread = prevThread;
    
      break;

    case WAIT:
     
      break;

    case EXIT:

      currentThread = NULL;
      currentThread = dequeue(&runningQueue);
      

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
      currentThread = dequeue(&runningQueue);

      if(currentThread == NULL)
      {
        printf("Error currentThread is NULL");
	      exit(EXIT_FAILURE);
      }
      
      break;
      
    case MUTEX_WAIT: //MUTEX_WAIT mutex lock

      //Don't add current to queue: already in mutex queue
      currentThread = dequeue(&runningQueue);


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
  timer.it_value.tv_usec = INTERVAL; //* (currentThread->priority + 1) ;
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


void garbage_collection()
{
  printf("Garbage collection!\n");
  operationInProgress = 1;
  currentThread->status = EXIT;
  
  //if no threads exit
  if(!mainContextInitialized) exit(EXIT_SUCCESS);

  tcb *jThread = NULL; //any threads waiting on the one being garbage collected
  node_t *jThreadNode = NULL;
  //dequeue all threads waiting on this one to finish
  while(!TAILQ_EMPTY(currentThread->joinQueue))
  {
    jThreadNode = TAILQ_FIRST(currentThread->joinQueue);
    jThread = jThreadNode->thread;

    TAILQ_REMOVE(currentThread->joinQueue, jThreadNode, nodes);
    free(jThreadNode);

    printf("Join queue of: %ld join: %ld\n", currentThread->tid, jThread->tid);
    jThread->retVal = currentThread->jVal;

    TAILQ_INSERT_HEAD(&runningQueue, jThreadNode, nodes);
  }

  //free stored node in allThreads
    node_t* i = NULL;
    TAILQ_FOREACH( i , &allThreads, nodes)
    {
      //Delete currentThread from allThreads
      if(i->thread->tid == currentThread->tid){
        break;
      }
    }
    TAILQ_REMOVE(&allThreads, i, nodes);
    free(i);
    free(currentThread->joinQueue); //double check

  operationInProgress = 0;

  raise(SIGVTALRM); //TODO: change to sigusr1
}


void initializeMainContext()
{
  tcb *mainThread = (tcb*)malloc(sizeof(tcb));
  ucontext_t *mText = (ucontext_t*)malloc(sizeof(ucontext_t));
  getcontext(mText);
  mText->uc_link = &cleanup;

  mainThread->context = mText;
  mainThread->tid = 0;
  mainThread->priority = 0;
  mainThread->jVal = NULL;
  mainThread->retVal = NULL;
  mainThread->status = READY;
  mainThread->joinQueue = malloc(sizeof(head_t*));
  TAILQ_INIT(mainThread->joinQueue);

  mainContextInitialized = 1;

  insert(&allThreads, mainThread);


  currentThread = mainThread;
  printf("Main context initialized!\n");
}

void initializeGarbageContext()
{
  //Set handler of timer
  signal(SIGVTALRM, scheduler);
  //set everything to NULL
  TAILQ_INIT(&allThreads);
  TAILQ_INIT(&runningQueue);
    
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
  newThread->jVal = NULL;
  newThread->retVal = NULL;
  newThread->status = READY;
  newThread->joinQueue = malloc(sizeof(head_t*));
  TAILQ_INIT(newThread->joinQueue);

  *thread = threadCount;
  threadCount++;
  //printf("Thread created: TID %ld\n", newThread->tid);

  //Enqueue newThread in running
  enqueue(&runningQueue, newThread);

  // Insert newThread in allThreads
  insert(&allThreads, newThread);

  operationInProgress = 0;

  //store main context
  if (!mainContextInitialized)
  {
    //printf("New context: TID %ld\n", newThread->tid);
    initializeMainContext();
    //raise(SIGVTALRM); //TODO
  }
  //printf("New thread created: TID %ld\n", newThread->tid);
  
  raise(SIGVTALRM);// TODO
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
  if(thread == currentThread->tid)  return -1;

  // Search the thread in allThreads
  tcb *tgt = NULL;
  struct node * i = NULL;
  TAILQ_FOREACH(i, &allThreads, nodes)
  {
    if (i->thread->tid == thread){
      tgt = i->thread;
      break;
    }
  }
  
  // if didn't find the thread
  if(tgt == NULL)   return -1;

  // Insert currentThread in the joinqueue of input thread
  insert( tgt->joinQueue, currentThread);

  currentThread->status = JOIN;

  operationInProgress = 0;
  raise(SIGVTALRM); // TODO

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