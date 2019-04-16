CC=gcc
DEPS= $(wildcard src/*.c) $(wildcard src/*.h)
default: webserver
webserver: $(DEPS)
		$(CC) src/webserver.c -o webserver -D$(TYPE) -lpthread -g -ggdb

create_n_threads: src/test_pthread.c src/my_pthread.c src/tcb.h src/list.c src/list.h
		$(CC) src/test_pthread.c src/my_pthread.c src/list.c -o create_n_threads -g -ggdb #-lpthread

.PHONY : clean
clean:
		rm webserver create_n_threads