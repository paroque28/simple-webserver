all: app

app: bclient.c  
		$(CC) bclient.c -o  bclient	 -lpthread 
		
.PHONY: clean



