CC=gcc
DEPS= $(wildcard src/*.c) $(wildcard src/*.h)
PTHREAD_DEPS=src/test_pthread.c src/my_pthread.c src/my_pthread_mutex.c src/my_pthread_debug.c src/list.c src/lottery.c
PTHREAD_HEADERS=src/my_pthread.h src/tcb.h src/list.h src/lottery.h
default: webserver
webserver: $(DEPS)
		$(CC) src/webserver.c -o webserver -D$(TYPE) -lpthread -g -ggdb

create_n_threads: $(PTHREAD_DEPS) $(PTHREAD_HEADERS)
		$(CC) $(PTHREAD_DEPS) -o create_n_threads -g -ggdb #-lpthread

.PHONY : clean
clean:
		rm webserver create_n_threads