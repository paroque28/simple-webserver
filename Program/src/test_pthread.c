#include <stdio.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "my_pthread.h"
#include <stdlib.h>
#include <signal.h>
#ifndef MY_PTHREAD
#include <pthread.h>
#endif



void test_sleep(unsigned long seconds){
    time_t start = time(NULL);
    while (1){
        time_t now = time(NULL);
        if(now >= start + 1 * seconds) return;
    }
}
pthread_mutex_t mutex1;
pthread_t tid[2000];
int run = 1;

//Threads routine
void* doSomeThing(void *arg)
{
    pthread_t* id = ((pthread_t*) arg);
    pthread_mutex_lock( &mutex1 );
    printf("Pthread: %ld\n", *id);
    pthread_mutex_unlock( &mutex1 );

    void * ptr = malloc(sizeof(int)*100); // Allocate 100 ints
    int i = 0;
    while(run){
        test_sleep(1);
        //printf("Mutexlock from thread %ld\n",*id);
        pthread_mutex_lock( &mutex1 );    
        printf("Hi #%d ", i++);
        printf("from thread ");
        printf("%ld.\n", *id);
        pthread_mutex_unlock( &mutex1 );  
        //printf("Mutex unlock from thread %ld\n",*id);
        test_sleep(1);
    }
    free(ptr);

    free(arg);

    return NULL;
}

 int main(int argc, char *argv[]) {
    int i = 0;
    int err;
    // Error if no number of threads specified
    if (argc != 2) return 1;
    my_pthread_setsched(RT_EDF);
    //Initialize mutex
    if (pthread_mutex_init(&mutex1, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    while(i < atoi(argv[1]))
    {
        pthread_t* number = malloc(sizeof (pthread_t));
        *number = (pthread_t) i + 1;
        pthread_attr_t thread_attr ;
        pthread_attr_init(&thread_attr);
        #ifdef MY_PTHREAD
        my_pthread_attr_set_tickets(&thread_attr,i+1);
        my_pthread_attr_set_rt_params(&thread_attr, 6, 2);
        #endif
        err = pthread_create(&(tid[i]), &thread_attr, &doSomeThing, (void *)number);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));

        i++;
    }

    
    //Wait one minute
    for( int i=0; i < 3; i++) {
        test_sleep(1);
        printf("\t######Sleep %d######\n", i + 1);
        //my_pthread_print_queues();
    }


    printf("Ending...\n");
    run = 0;
    i = 0;
    //Wait for all threads to exit
    while(i < atoi(argv[1]))
    {
        pthread_join( tid[i] , NULL);
        i++;
    }
    pthread_mutex_destroy(&mutex1);
    return 0;
 }

