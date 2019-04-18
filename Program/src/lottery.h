#ifndef LOTTERY_H
#define LOTTRY_H

#include <sys/queue.h>
#include "tcb.h"
#include "list.h"

tcb* lotterygetNextThread(head_t*);
unsigned int getNumberOfTickets(head_t*);
unsigned int randomIndex(unsigned int range);

#endif