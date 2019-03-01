#  Makefile for server
default: webserver
webserver: src/webserver.c
		gcc -g -ggdb src/webserver.c -o webserver
.PHONY : clean
clean:
		rm webserver