all: app

app: bclient.c  ./libmy_pthread.a
		$(CC)  -o  bclient bclient.c ./libmy_pthread.a   -lpthread  -lm -I./

		
.PHONY: clean



