#include "my_pthread.h"


void getNextThread(){
  //printf("Algorithm: %d\n",schedulingAlgorithm);
  if(schedulingAlgorithm == RR){
    // Dequeue new thread from runningQueue
    currentThread = dequeue(&runningQueue);
  }
  else if(schedulingAlgorithm == SELFISH_RR){
    currentThread = selfishRRgetNextThread(&runningQueue, &newQueue);
  }
  else if(schedulingAlgorithm == LOTTERY){
    currentThread = lotterygetNextThread(&runningQueue);
  }
  else if(schedulingAlgorithm == RT_EDF){
    currentThread = EDFgetNextThread(&runningQueue);
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
    //printf("Operation in Progress!\n");
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
      printf("Excided maximum number of ticks!!\n We didn't expect to see you +500 years in the future\n");
      exit(signum);
    }
    selfishUpdateScores(&runningQueue, &newQueue);
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

      if(currentThread == NULL && schedulingAlgorithm != SELFISH_RR)
      {
        printf("Error currentThread is NULL");
	      exit(EXIT_FAILURE);
      }
      //Selfish RR to prevent this error enquue the main thread again
      // else if(schedulingAlgorithm == SELFISH_RR){
      //   currentThread = prevThread;
      //   enqueue(&runningQueue, currentThread);
      // }
      
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


void my_pthread_setsched(int newSchedulingAlgorithm){
  if(!mainContextInitialized)
  {
    initializeGarbageContext();
    initializeMainContext();
  }
  schedulingAlgorithm = newSchedulingAlgorithm;
  srand(time(0)); 
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