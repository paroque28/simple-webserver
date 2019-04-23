#include "lottery.h"

// Lottery scheduling get next
tcb* lotterygetNextThread(head_t* runningQueue){
    tcb* next = NULL;
    unsigned int count = 0;
    unsigned int totalTickets = getNumberOfTickets(runningQueue);
    // GET the random index
    unsigned int index = randomIndex(totalTickets);

    //printf("Random: %d of %d\n", index+1, totalTickets);
    node_t* i = NULL;
    // Find the element next in the random queue
    TAILQ_FOREACH(i, runningQueue, nodes)
    {
        count += (i->thread->tickets + 1);
        if(count >= index + 1){
            //printf("Found next %ld, index %d, count %d\n", i->thread->tid, index, count);
            next = i->thread;
            // Unqueue the element and return
            TAILQ_REMOVE(runningQueue, i, nodes);
            free(i);
            break;
        }
    }
    //printf("Next: %ld\n", next->tid);
    return next;
}
// How many tickets in total
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
//Get random in range
unsigned int randomIndex(unsigned int range){
    return (rand() % range );
}