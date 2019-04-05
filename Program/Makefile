CC=gcc
DEPS= $(wildcard src/*.c) $(wildcard src/*.h)
default: webserver
webserver: $(DEPS)
		$(CC) src/webserver.c -o webserver -D$(TYPE) -lpthread -g -ggdb

.PHONY : clean
clean:
		rm webserver