#include "my_pthread.h"

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
        TAILQ_REMOVE(&allThreads, i, nodes);
        free(i);
        break;
      }
    }
    
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
  mainThread->selfishScore = 0;
  mainThread->joinQueue = malloc(sizeof(head_t*));
  initQueue(mainThread->joinQueue);

  mainThread->tickets = 2;
  mainThread->duration = 4;
  mainThread->period = 100;
  mainThread->quantumsRun = 0;
  mainThread->lastRun = 0;

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
  initQueue(&newQueue);
    
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
  newThread->selfishScore = 0;
  newThread->quantumsRun = 0;
  newThread->lastRun = 0;
  newThread->joinQueue = malloc(sizeof(head_t*));
  initQueue(newThread->joinQueue);
  if (attr == NULL){
    newThread->tickets = DEFAULT_TICKETS;
    newThread->duration = DEFAULT_DURATION;
    newThread->period = DEFAULT_PERIOD;
  }
  else{
    newThread->tickets = attr->tickets;
    newThread->duration = attr->duration;
    newThread->period = attr->period;
  }

  *thread = threadCount;
  threadCount++;
  //printf("Thread created: TID %ld with %d ticket(s)\n", newThread->tid,newThread->tickets + 1);

  //Enqueue newThread in running
  if(schedulingAlgorithm == SELFISH_RR){
    enqueue(&newQueue, newThread);
  }
  else{
    enqueue(&runningQueue, newThread);
  }

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

int my_pthread_attr_init (my_pthread_attr_t* attr){
  attr->tickets = DEFAULT_TICKETS;
  attr->period = DEFAULT_PERIOD;
  attr->duration = DEFAULT_DURATION;
  return 0;
}
int my_pthread_attr_set_tickets(my_pthread_attr_t* attr, unsigned int tickets){
  if(tickets == 0) attr->tickets = 0;
  else attr->tickets = tickets - 1;
}
int my_pthread_attr_set_rt_params(my_pthread_attr_t* attr, unsigned int period, unsigned int duration){
  attr->period = period;
  attr->duration = duration;
  return 0;
}

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
  if(thread == currentThread->tid) {
    //printf("Thread waiting on self\n");
    return -1;
  }
  
  
  

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
  {
    return 0;
  }

  *value_ptr = currentThread->retVal;
  return 0;
};

int my_pthread_detach(my_pthread_t thread)
{
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  operationInProgress = 1;

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

  if(currentThread->tid == tgt->tid){
    getNextThread();
  }

  removeFromQueue(&runningQueue, tgt);

  tgt->status = DETACH;

  operationInProgress = 0;
  raise(SIGUSR1);

  return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr)
{
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  // call garbage collection
  currentThread->jVal = value_ptr;
  setcontext(&cleanup);
};


//Busy waiting
void my_sleep(unsigned long seconds){
    time_t start = time(NULL);
    while (1){
        time_t now = time(NULL);
        if(now >= start + 1 * seconds) return;
    }
}
