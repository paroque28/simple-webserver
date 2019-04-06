#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
//#include <pthread.h>
#include "my_pthread.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

//pthread_mutex_t mutex1;
pthread_t tid[2000];
int run = 1;

//Print Pointed Identifier
void printPt(pthread_t pt) {
    unsigned char *ptc = (unsigned char*)(void*)(&pt);
    printf("Pthread: ");
    printf("0x");
    for (size_t i=0; i<sizeof(pt); i++) {
        printf( "%02x", (unsigned)(ptc[i]));
    }
    printf("\n");
}

//THreads routine
void* doSomeThing(void *arg)
{
    pthread_t* id = ((pthread_t*) arg);
    
    //pthread_mutex_lock( &mutex1 );
    printPt(*id);
    //pthread_mutex_unlock( &mutex1 );


    void * ptr = malloc(sizeof(int)*100); // Allocate 100 ints
    while(run); //Infinite Loop
    free(ptr);

    free(arg);

    return NULL;
}

 int main(int argc, char *argv[]) {
    printf("Starting. Pid: %d\n", (int)getpid());
    int i = 0;
    int err;
    // Error if no number of threads specified
    if (argc != 2) return 1;
    while(i < atoi(argv[1]))
    {
        pthread_t* number = malloc(sizeof (pthread_t));
        *number = (pthread_t) i;
        err = pthread_create(&(tid[i]), NULL, &doSomeThing, (void *)number);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));

        i++;
    }

    raise(SIGVTALRM);
    //Wait one minute
    for( int i=0; i < 5; i++) {
        printf("Sleep %d\n", i );
        sleep(1);
        
    }


    printf("Ending\n");
    run = 0;
    i = 0;
    //Wait for all threads to exit
    while(i < atoi(argv[1]))
    {
        pthread_join( tid[i] , NULL);
        i++;
    }

    return 0;
 }

