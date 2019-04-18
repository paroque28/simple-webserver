#include "selfishrr.h"

tcb* selfishRRgetNextThread(head_t* runningQueue, head_t* newQueue){
    return dequeue(runningQueue);
}


void selfishUpdateScores(head_t* runningQueue, head_t* newQueue){
    //printf("Updating SelfishRR scores!\n");
    node_t* i = NULL;
    unsigned long maxNewQueue = 0;
    unsigned long minRunningQueue = -1;

    //Update Scores of both
    TAILQ_FOREACH(i, newQueue, nodes)
    {
        i->thread->selfishScore += RATE_A;
        //Update max
        if(i->thread->selfishScore > maxNewQueue) maxNewQueue = i->thread->selfishScore;
    }
    unsigned int size = 0;
    TAILQ_FOREACH(i, runningQueue, nodes)
    {
        i->thread->selfishScore += RATE_B;
        //Update min
        //printf("score: %ld\n",i->thread->selfishScore);
        if(i->thread->selfishScore < minRunningQueue) minRunningQueue = i->thread->selfishScore;
        size ++;
    }
    //Check if someone from new queue reached the accepted queue
    //printf("MinRun: %ld, MaxNew %ld\n", minRunningQueue, maxNewQueue);
    TAILQ_FOREACH(i, newQueue, nodes){   
        //Check for empy running list
        if(TAILQ_EMPTY(runningQueue)){
            if(i->thread->selfishScore == maxNewQueue){
                minRunningQueue = maxNewQueue;

            }
        }

        if(i->thread->selfishScore >= minRunningQueue){
            tcb* promoted = i->thread;
            TAILQ_REMOVE(newQueue, i, nodes);
            free(i);
            enqueue(runningQueue,promoted);
            printf("Thread: %ld promoted!\n", promoted->tid);
        }
    }


    //Prevent queue top score going to high
    if (size == 1 && i != NULL && i->thread != NULL && i->thread->tid == 0){
        i->thread->selfishScore = maxNewQueue;
    }
}