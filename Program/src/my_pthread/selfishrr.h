#ifndef SELFISHRR_H
#define SELFISHRR_H

#include <sys/queue.h>
#include "tcb.h"
#include "list.h"


// Adjusting the parameters a and b : 
//           -> If b / a >= 1, a new process is not accepted 
//                  until all the accepted processes have finished, so SRR becomes FCFS. 
//           -> If b / a = 0, all processes are accepted immediately, so SRR becomes RR. 
//           -> If 0 < b / a < 1, accepted processes are selfish, but not completely.
 #define RATE_A 1 // New Threads
 #define RATE_B 2 // Accepted Threads

tcb* selfishRRgetNextThread(head_t* runningQueue, head_t* newQueue);
void selfishUpdateScores(head_t* runningQueue, head_t* newQueue);

#endif