#include "my_pthread.h"

void getNextThread(){
  if(schedulingAlgorithm == RR){
    // Dequeue new thread from runningQueue
    currentThread = dequeue(&runningQueue);
  }
  else if(schedulingAlgorithm == SELFISH_RR){
    printf("Scheduling alghrithm no implemented!\n");
    exit(1);
  }
  else if(schedulingAlgorithm == LOTTERY){
    printf("Scheduling alghrithm no implemented!\n");
    exit(1);
  }
  else if(schedulingAlgorithm == RT_EDF){
    printf("Scheduling alghrithm no implemented!\n");
    exit(1);
  }
  else{
    printf("No scheduling alghrithm selected!\n");
    exit(1);
  }
}

void scheduler(int signum)
{
  if(operationInProgress)
  {
    printf("Operation in Progress!\n");
    return;
  }
  operationInProgress = 1;
  //Record remaining time
  getitimer(ITIMER_VIRTUAL, &currentTime);

  //printf("\n[Thread %ld] Signaled from %ld, time left %i\n", currentThread->tid,currentThread->tid, (int)currentTime.it_value.tv_usec);
  
  if(signum != SIGVTALRM && signum != SIGUSR1 && signum != SIGUSR2)
  {
    printf("[Thread %ld] Signal Received: %d.\nExiting...\n", currentThread->tid, signum);
    exit(signum);
  }
  if(signum == SIGVTALRM){
    ticks ++;
    if(ticks == -1){
      printf("Excided maximum number of ticks!!\n");
      exit(signum);
    }
    //printf("Tick!\n");
  }
  // /if(signum == SIGUSR2) printf("Scheduler Triggered from Mutex!\n");

  // Time elapsed = difference between max interval size and time remaining in timer
  // if the time splice doesn't deplates the else body goes 
  // the actual amount of time that passed is added to timeelapsed
  int timeSpent = (int)currentTime.it_value.tv_usec;
  //printf("timeSpent: %i\n", timeSpent);
  if(timeSpent < 0 || timeSpent > TICK)
  {
    timeSpent = 0;
  }
  else
  {
    timeSpent = TICK - timeSpent;
  }

  
  timeElapsed += timeSpent;

  //printf("[Thread %ld] Total time: %d from time remaining: %d out of %d\n", currentThread->tid, timeElapsed, (int)currentTime.it_value.tv_usec, INTERVAL);

  prevThread = currentThread;

  int i;
  switch(currentThread->status)
  {
    case READY: //READY current thread is in the running queue

      //put back the thread that just finished back into the running queue
      enqueue(&runningQueue, currentThread);
      getNextThread();


      break;
   
    case YIELD: //YIELD pthread yield was called; 
      getNextThread();

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
      getNextThread();
      

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

      //printf("-Switching to: TID %ld\n", currentThread->tid );

      reset_timer();
      operationInProgress = 0;
      setcontext(currentThread->context);

      break;

    case JOIN: //JOIN pthread_join

      currentThread = NULL;
      //notice how we don't enqueue the thread that just finished back into the running queue
      //we just go straight to getting another thread
      getNextThread();

      if(currentThread == NULL)
      {
        printf("Error currentThread is NULL");
	      exit(EXIT_FAILURE);
      }
      
      break;
      
    case MUTEX_WAIT: //Wating for mutex lock
      //printf("Status Mutex wait current: %ld\n", currentThread->tid);
      //my_pthread_print_queues();
      //Don't add current to queue: already in mutex queue
      getNextThread();


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
  if(currentThread == NULL)
  {

    printf("Current Thread NULL, prevThread %ld\n Need to investigate error!\n", prevThread->tid);
    exit(1);
  }
  currentThread->status = READY;

  reset_timer();
  operationInProgress= 0;
  
  //Switch to new context
  if(prevThread->tid == currentThread->tid)  
  {
      //printf("Switching to same thread: TID %ld\n", currentThread->tid);
  }
  else
  {
    //printf("Switching to: TID %ld\n", currentThread->tid);
    swapcontext(prevThread->context, currentThread->context);
  }
  //printf("Scheduler Out!\n");
  return;

}


void garbage_collection()
{
  //printf("Garbage collection!\n");
  operationInProgress = 1;
  currentThread->status = EXIT;
  
  //if no threads exit
  if(!mainContextInitialized) exit(EXIT_SUCCESS);

  tcb *jThread = NULL; //any threads waiting on the one being garbage collected
  node_t *jThreadNode = NULL;
  //dequeue all threads waiting on this one to finish
  while(!TAILQ_EMPTY(currentThread->joinQueue))
  {
    jThread = dequeue(currentThread->joinQueue);

    //printf("Join queue of: %ld join: %ld\n", currentThread->tid, jThread->tid);
    jThread->retVal = currentThread->jVal;

    insert(&runningQueue, jThread);
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

  raise(SIGUSR1);
}


void initializeMainContext()
{
  schedulingAlgorithm = RR;
  tcb *mainThread = (tcb*)malloc(sizeof(tcb));
  ucontext_t *mText = (ucontext_t*)malloc(sizeof(ucontext_t));
  getcontext(mText);
  mText->uc_link = &cleanup;

  mainThread->context = mText;
  mainThread->tid = 0;
  mainThread->jVal = NULL;
  mainThread->retVal = NULL;
  mainThread->status = READY;
  mainThread->tickets = 2;
  mainThread->joinQueue = malloc(sizeof(head_t*));
  initQueue(mainThread->joinQueue);

  mainContextInitialized = 1;

  enqueue(&allThreads, mainThread);


  currentThread = mainThread;
  ticks = 0;
}

void initializeGarbageContext()
{
  //Set handler of timer
  signal(SIGVTALRM, scheduler);
  signal(SIGUSR1, scheduler);
  signal(SIGUSR2, scheduler);
  //set everything to NULL
  initQueue(&allThreads);
  initQueue(&runningQueue);
    
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
int my_pthread_create(my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg)
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
  newThread->jVal = NULL;
  newThread->retVal = NULL;
  newThread->status = READY;
  newThread->tickets = 0;
  newThread->joinQueue = malloc(sizeof(head_t*));
  initQueue(newThread->joinQueue);

  *thread = threadCount;
  threadCount++;
  printf("Thread created: TID %ld with %d ticket(s)\n", newThread->tid,newThread->tickets + 1);

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
    //raise(SIGUSR1); 
  }
  //printf("New thread created: TID %ld\n", newThread->tid);
  
  raise(SIGUSR1);
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
  raise(SIGUSR1);

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


void reset_timer(){
  timer.it_value.tv_sec = INTERVAL_SEC * QUANTUM;
  timer.it_value.tv_usec = INTERVAL_MICROSEC * QUANTUM;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  int ret = setitimer(ITIMER_VIRTUAL, &timer, NULL);

  if (ret < 0)
  {
     printf("Timer Reset Failure. Exiting...\n");
     exit(0);
  }
}