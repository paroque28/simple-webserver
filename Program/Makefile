CC=gcc
DEPS= $(wildcard src/*.c) $(wildcard src/*.h)
default: webserver
webserver: $(DEPS)
		$(CC) -g -ggdb src/webserver.c -o webserver -D$(TYPE)

.PHONY : clean
clean:
		rm webserver