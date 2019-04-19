#ifndef EDF_H
#define EDF_H
// Earliest deadline first scheduling
#define DEFAULT_DURATION 1
#define DEFAULT_PERIOD 1000

#include <sys/queue.h>
#include "tcb.h"
#include "list.h"

tcb* EDFgetNextThread(head_t* runningQueue, long long now, tcb* currentThread);

#endif