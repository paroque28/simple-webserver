CC=gcc
DEPS= $(wildcard src/*.c) $(wildcard src/*.h) $(wildcard src/my_pthread/*.c) $(wildcard src/my_pthread/*.h)
PTHREAD_DEPS=src/my_pthread/my_pthread.c src/my_pthread/my_pthread_mutex.c src/my_pthread/my_pthread_debug.c src/my_pthread/list.c src/my_pthread/lottery.c src/my_pthread/selfishrr.c src/my_pthread/edf.c src/my_pthread/scheduling.c
PTHREAD_HEADERS=src/my_pthread/my_pthread.h src/my_pthread/tcb.h src/my_pthread/list.h src/my_pthread/lottery.h src/my_pthread/selfishrr.h src/my_pthread/edf.h
default: webserver
webserver: $(DEPS)
		if [ -z "$(TYPE)" ]; then error "TYPE is not set"; fi
		$(CC) src/webserver.c -o webserver -D$(TYPE) -g -ggdb -lpthread

create_n_threads: $(PTHREAD_DEPS) $(PTHREAD_HEADERS)
		$(CC) src/test_pthread.c $(PTHREAD_DEPS) -o create_n_threads -g -ggdb #-lpthread

.PHONY : clean
clean:
		rm webserver create_n_threads