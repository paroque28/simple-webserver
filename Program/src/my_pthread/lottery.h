#ifndef LOTTERY_H
#define LOTTERY_H

#define DEFAULT_TICKETS 0

#include <sys/queue.h>
#include "tcb.h"
#include "list.h"

tcb* lotterygetNextThread(head_t*);
unsigned int getNumberOfTickets(head_t*);
unsigned int randomIndex(unsigned int range);

#endif