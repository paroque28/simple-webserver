#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <stdbool.h>


#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG   44
#define MAX_THREADS 1000

//#define PRETHREADED


#ifdef FIFO
#define TYPE_SELECTED
#endif
#ifdef FORK
#define TYPE_SELECTED
#endif
#ifdef THREADED
#define TYPE_SELECTED
#endif
#ifdef PRETHREADED
#define TYPE_SELECTED
#endif
#ifdef PREFORK
#define TYPE_SELECTED
#endif

#ifndef TYPE_SELECTED
#error You must define FORK or FIFO or THREADED or PREFORK or PRETHREADED before compiling webserver use -D flag on gcc or TYPE= on make
#endif

#if defined(THREADED) || defined(PRETHREADED)

	#ifdef MY_PTHREAD_LIB
		#include "my_pthread/my_pthread.h"
	#endif
	#ifndef MY_PTHREAD_LIB
		#include <pthread.h>
	#endif

#endif


typedef struct web_args {
    int fd;
    int hit;
	int thread_id;  //or fork
} web_args_t;

typedef struct types{
	char *ext;
	char *filetype;
} types_t;



//#######Variables###############
extern types_t extensions[];



//#######Functions##############


#endif