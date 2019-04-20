CC=gcc

PTHREAD_DEPS=src/my_pthread/my_pthread.c src/my_pthread/my_pthread_mutex.c src/my_pthread/my_pthread_debug.c src/my_pthread/list.c src/my_pthread/lottery.c src/my_pthread/selfishrr.c src/my_pthread/edf.c src/my_pthread/scheduling.c
PTHREAD_HEADERS=src/my_pthread/my_pthread.h src/my_pthread/tcb.h src/my_pthread/list.h src/my_pthread/lottery.h src/my_pthread/selfishrr.h src/my_pthread/edf.h

WEBSERVER_DEPS=src/webserver.c 
WEBSERVER_HEADERS=src/webserver.h

default: webserver

webserver: src/main.c $(WEBSERVER_DEPS) $(WEBSERVER_HEADERS)
		if [ -z "$(TYPE)" ]; then error "TYPE is not set"; fi
		$(CC) src/main.c $(WEBSERVER_DEPS) -o webserver -D$(TYPE) -g -ggdb -lpthread

create_n_threads: src/test_pthread.c $(PTHREAD_DEPS) $(PTHREAD_HEADERS)
		$(CC) src/test_pthread.c $(PTHREAD_DEPS) -o create_n_threads -g -ggdb #-lpthread

.PHONY : clean
clean:
		rm webserver create_n_threads