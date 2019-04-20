CC=gcc
DEPS= $(wildcard src/*.c) $(wildcard src/*.h)
PTHREAD_DEPS=src/my_pthread.c src/my_pthread_mutex.c src/my_pthread_debug.c src/list.c src/lottery.c src/selfishrr.c src/edf.c src/scheduling.c
PTHREAD_HEADERS=src/my_pthread.h src/tcb.h src/list.h src/lottery.h src/selfishrr.h src/edf.h
default: webserver
webserver: $(DEPS)
		$(CC) src/webserver.c -o webserver -D$(TYPE) -g -ggdb -lpthread

create_n_threads: $(PTHREAD_DEPS) $(PTHREAD_HEADERS)
		$(CC) src/test_pthread.c $(PTHREAD_DEPS) -o create_n_threads -g -ggdb #-lpthread

.PHONY : clean
clean:
		rm webserver create_n_threads