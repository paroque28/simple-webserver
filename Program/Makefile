CC=gcc

CFLAGS=-g -ggdb
ifeq (${THREAD_LIB}, PTHREAD)
CFLAGS += -lpthread
endif
ifeq (${THREAD_LIB}, MY_PTHREAD)
CFLAGS += -D MY_PTHREAD_LIB
endif

PTHREAD_DEPS=src/my_pthread/my_pthread.c src/my_pthread/my_pthread_mutex.c src/my_pthread/my_pthread_debug.c src/my_pthread/list.c src/my_pthread/lottery.c src/my_pthread/selfishrr.c src/my_pthread/edf.c src/my_pthread/scheduling.c
PTHREAD_HEADERS=src/my_pthread/my_pthread.h src/my_pthread/tcb.h src/my_pthread/list.h src/my_pthread/lottery.h src/my_pthread/selfishrr.h src/my_pthread/edf.h

WEBSERVER_DEPS=src/webserver.c 
WEBSERVER_HEADERS=src/webserver.h

THREAD_LIB=PTHREAD
SCHEDULER=LOTTERY

default: webserver

webserver: src/main.c $(WEBSERVER_DEPS) $(WEBSERVER_HEADERS) $(PTHREAD_DEPS) $(PTHREAD_HEADERS)
		@if [ -z "$(TYPE)" ]; then echo "TYPE is not set"; exit 1; fi
		@if [ -z "$(THREAD_LIB)" ]; then echo "THREAD_LIB is not set"; exit 1; fi
		@if ! ( [ "$(SCHEDULER)" = "RR" -o "$(SCHEDULER)" = "SELFISH_RR" -o "$(SCHEDULER)" = "LOTTERY" -o "$(SCHEDULER)" = "RT_EDF" ] ); then \
			echo "SCHEDULER is not set"; exit 1; fi

		@if ! [ "$(THREAD_LIB)" = "PTHREAD" -o "$(THREAD_LIB)" = "MY_PTHREAD" ]; then \
			echo "THREAD_LIB: $(THREAD_LIB) is not set correctly"; exit 1; \
		fi
		$(CC) src/main.c  $(WEBSERVER_DEPS) $(PTHREAD_DEPS) -o webserver -D $(TYPE) -D SCHEDULER=$(SCHEDULER) $(CFLAGS);


create_n_threads: src/test_pthread.c $(PTHREAD_DEPS) $(PTHREAD_HEADERS)
		$(CC) src/test_pthread.c $(PTHREAD_DEPS) -o create_n_threads $(CFLAGS)

.PHONY : clean
clean:
		rm webserver create_n_threads