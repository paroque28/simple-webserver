#ifndef EDF_H
#define EDF_H

#include <sys/queue.h>
#include "tcb.h"
#include "list.h"

tcb* EDFgetNextThread(head_t* runningQueue);

#endif