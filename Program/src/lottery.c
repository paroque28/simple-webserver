#include "lottery.h"


tcb* lotterygetNextThread(head_t* runningQueue){
    tcb* next = NULL;
    unsigned int count = 0;
    unsigned int totalTickets = getNumberOfTickets(runningQueue);
    unsigned int index = randomIndex(totalTickets);
    //printf("Random: %d of %d\n", index+1, totalTickets);
    node_t* i = NULL;
    TAILQ_FOREACH(i, runningQueue, nodes)
    {
        count += (i->thread->tickets + 1);
        if(count >= index + 1){
            //printf("Found next %ld, index %d, count %d\n", i->thread->tid, index, count);
            next = i->thread;
            TAILQ_REMOVE(runningQueue, i, nodes);
            free(i);
            break;
        }
    }
    //printf("Next: %ld\n", next->tid);
    return next;
}
unsigned int getNumberOfTickets(head_t* queue){
    unsigned int count = 0;

    node_t* i = NULL;
    TAILQ_FOREACH(i, queue, nodes)
    {
        // tickets start at zero
        count += (i->thread->tickets + 1);
    }

    return count;
}
unsigned int randomIndex(unsigned int range){
    return (rand() % range );
}