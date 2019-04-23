#include "edf.h"
// Earliest deadline first scheduling
tcb* EDFgetNextThread(head_t* runningQueue, long long now, tcb* currentThread){
    node_t* i = NULL;
    long long earliestDeadline = __LONG_LONG_MAX__;
    tcb* earliestDeadlineThread = NULL;

    // Make sure thread are compliant with duration
    if(currentThread != NULL){
        if(now > currentThread->lastRun + currentThread->duration){
            return currentThread;
        }
    }
    
    // Find the earliest deadline 
    TAILQ_FOREACH(i, runningQueue, nodes)
    {   
        long long deadline = (i->thread->lastRun + i->thread->period);
        long long timeToDeadline = deadline - now;
        if(timeToDeadline < earliestDeadline){
            earliestDeadline = timeToDeadline;
            earliestDeadlineThread = i->thread;
        }
    }
    //unqueue
    removeFromQueue(runningQueue,earliestDeadlineThread);

    if(earliestDeadline < 0) printf("RealTime policies couldn't be met! Continuing as Soft RT\n");
    //Update last run
    earliestDeadlineThread->lastRun = now;

    return earliestDeadlineThread;
}