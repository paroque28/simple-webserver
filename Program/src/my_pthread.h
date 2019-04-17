#ifndef MY_PTHREAD_H
#define MY_PTHREAD_H


/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <malloc.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include "tcb.h"
#include "list.h"

#define READY 0
#define YIELD 1
#define WAIT 2
#define EXIT 3
#define JOIN 4

#define INTERVAL 10000 // HALF second

typedef struct sigaction mySig;

// TOOLS ----------------
void garbage_collection();
void initializeMainContext();
void initializeGarbageContext();

// DEBUG ----------------
void my_pthread_print_queues();


// THREAD ----------------
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);
/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();
/* terminate a thread */
void my_pthread_exit(void *value_ptr);
/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);



// MUTEX  ---------------------


/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);
/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);



// FIX sleep
void my_sleep(unsigned long time);



#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create(a, b, c, d) my_pthread_create(a, b, c, d)
#define pthread_yield() my_pthread_yield()
#define pthread_exit(a) my_pthread_exit(a)
#define pthread_join(a, b) my_pthread_join(a, b)
#define pthread_mutex_init(a, b) my_pthread_mutex_init(a, b)
#define pthread_mutex_lock(a) my_pthread_mutex_lock(a)
#define pthread_mutex_unlock(a) my_pthread_mutex_unlock(a)
#define pthread_mutex_destroy(a) my_pthread_mutex_destroy(a)



// Variables

tcb *currentThread, *prevThread;
head_t runningQueue;
head_t allThreads;
head_t tickets;
ucontext_t cleanup;
sigset_t signal_set;
mySig sig;

struct itimerval timer, currentTime;

int mainContextInitialized;
int timeElapsed;
int threadCount;
int operationInProgress;

#endif
