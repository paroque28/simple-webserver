#include <stdio.h> //Used to write the beanchmark
#include <stdlib.h> //System functions
#include <string.h> //Used for parse args inserted by the user
#include <pthread.h> //Used for creating threads.
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <time.h> //Used to measure the response and total time.
#include <netdb.h> //Used to communicate with the server.

/*Constants*/
#define benchmarkName "BenchMark.csv"
#define  bufferSize 1024

void* downloadFile(void *data);

//The client send a querry to the server
int ReadHttpStatus(int sock){
    char c;
    char buff[bufferSize]="",*ptr=buff+1;
    int bytes_received, status;
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("ReadHttpStatus");
            exit(1);
        }
        if((ptr[-1]=='\r')  && (*ptr=='\n' )) break;
        ptr++;
    }
    *ptr=0;
    ptr=buff+1;
    sscanf(ptr,"%*s %d ", &status);
    return (bytes_received>0)?status:0;

}

/*
 *This function deal with the size of the file
 *Stop as soon as all the bytes are transferred. 
 */
int ParseHeader(int sock){
    char c;
    char buff[bufferSize]="",*ptr=buff+4;
    int bytes_received, status;
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("Parse Header");
            exit(1);
        }
        if((ptr[-3]=='\r')  && (ptr[-2]=='\n' ) && (ptr[-1]=='\r')  && (*ptr=='\n' ))
			break;
        ptr++;
    }

    *ptr=0;
    ptr=buff+4;
    if(bytes_received){
        ptr=strstr(ptr,"Content-Length:");
        if(ptr){
            sscanf(ptr,"%*s %d",&bytes_received);

        }else
            bytes_received=-1; //unknown size

    }
    return  bytes_received ;

}

/*Handle the most important info of each execution
* It is helpful to notice what thread is running anywhere in the code*/

typedef struct thread_arg {
	int  nthread; //Thread number
	int  ncycle; //Cycle
	char domain[30]; //Main address
	char path[30]; //File name
	int port; //File name
	
}thread_arg;



int main(int argc, char *argv[]){
 
	// # bclient <machine> <port> <file> <N-threads> <N-cycles>
	
	clock_t t; //Total Execution time
	clock_t t2; //Total Execution time
   
     
	if( argc ==6){/*parameters*/
		char machine[20];
		char file[20];
		int port=0;
		int threads=0;
		int cycles=0;
		
		/*Parsing arguments*/
		strcpy(machine, argv[1]);
		strcpy(file, argv[3]);
		threads=atoi(argv[4]);
		port=atoi(argv[2]);
		cycles=atoi(argv[5]);
		
		
		FILE* fb=fopen(benchmarkName,"wb");
		//fwrite(recv_data,1,bytes_received,fb);
		
		/* print parameters*/
		fprintf(fb,"Machine, %s\n", machine); // 
		fprintf(fb,"file, %s\n", file); // 
		fprintf(fb,"Port, %d\n", port); // 
		fprintf(fb,"Threads,  %d\n", threads); // 
		fprintf(fb,"Cycles, %d\n", cycles); // 
		fprintf(fb,"URL, %s:%d/%s\n", machine,port, file);
		fclose (fb);
		
		pthread_t thread_id [threads];  //PTHREAD 1/4
		//my_pthread_t thread_id[threads];//MYPTHREAD 1/4
		struct thread_arg arg[threads];
		
		// t = clock();
		clock_t t2 = clock(); 
    		/* Creating pthread*/
		for(int i=0;i<threads;i++){
			arg[i].nthread = i+1;
			arg[i].ncycle=cycles;

			strcpy(arg[i].domain, machine);
			strcpy(arg[i].path,file);
			arg[i].port=port;

			pthread_create(&thread_id [i], NULL, &downloadFile, &arg[i]);//PTHREAD 2/4
		    //my_pthread_create(&thread_id[i], NULL, &downloadFile, &arg[i]); //MYPTHREAD 2/4


			//arg[i].id = thread_id[i];
			// printf("In main \nthread id = %ld\n", (long)  arg[i].id );
			//printf("In main nthread id = %d\n",  arg.nthread );

		} 
		
		for(int i=0;i<threads;i++){
			pthread_join(thread_id [i], NULL);  //PTHREAD 3/4
			//my_pthread_join(thread_id[i],NULL);	 //MYPTHREAD 3/4
				
		}
		t2 = clock() - t2; 
				double time_taken = ((double)t2)/CLOCKS_PER_SEC; // in seconds 
				fb=fopen(benchmarkName,"a");	
				fprintf(fb, "Total execution time, %f\n", time_taken);
				fclose(fb);

		
		  
	
	  
		//pthread_exit(NULL); //PTHREAD 4/4
		//my_pthread_exit(NULL);//MYPTHREAD 4/4	
		}
	return 0;
}




/*Make the query and download the specified file, 
 * It also measures the response time of each cycle of each thread*/
void* downloadFile(void *data){
	
	struct thread_arg *arg = data;
	
	char domain[30];
	char path[30];
	int port;

	port=arg->port;

	//sprintf(domain, "%s:%d",  arg->domain, port);
		sprintf(domain, "%s",  arg->domain);
	 strcpy(path,arg->path);

	
	
	/*Iterates the cycles*/
	for (int j=1;j<=arg->ncycle;j++){
		char thID[30];
		sprintf(thID, "T%d-C%d",arg->nthread, j);
		int sock, bytes_received;  
		char send_data[bufferSize],recv_data[bufferSize], *p;
		struct sockaddr_in server_addr;
		struct hostent *he;
		he = gethostbyname(domain);
		if (he == NULL){
		   herror(domain);
		   herror("gethostbyname");
		   exit(1);
		}
		if ((sock = socket(AF_INET, SOCK_STREAM, 0))== -1){
		   perror("Socket");
		   exit(1);
		}
		server_addr.sin_family = AF_INET;     
		server_addr.sin_port = htons(port);
		server_addr.sin_addr = *((struct in_addr *)he->h_addr);
		bzero(&(server_addr.sin_zero),8); 
		
		if (connect(sock, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
		   perror("Connect");
		   exit(1); 
		}
		snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);
		if(send(sock, send_data, strlen(send_data), 0)==-1){
			perror("send");
			exit(2); 
		}
		int contentlengh;
		clock_t t; 
		/*Calculate the time that the server takes to response*/
		t = clock(); 
		int auxBytes=0;
		if(ReadHttpStatus(sock) && (contentlengh=ParseHeader(sock))){
			t = clock() - t; 
			double time_taken = ((double)t)/CLOCKS_PER_SEC; // in second
			FILE* fb=fopen(benchmarkName,"a");//	
			fprintf(fb, "Response time %s,  %f\n",thID, time_taken);//Response time of each cycle of each thread.
			fclose(fb);
		int bytes=0;
		//strcat(thID,path); //Unmark this line if you want to save the file in your computer. Line 1/4
		//FILE* fd=fopen(thID,"wb"); //Unmark this line if you want to save the file in your computer. Line 2/4

		while(bytes_received = recv(sock, recv_data, bufferSize, 0)){
			if(bytes_received==-1){
				perror("recieve");
				exit(3);
			}
			//fwrite(recv_data,1,bytes_received,fd);//Unmark this line if you want to save the file in your computer. Line 3/4
			bytes+=bytes_received;
			auxBytes+=bytes_received;

			if(bytes==contentlengh)	
				break;

		}
			//fclose(fd);//Unmark this line if you want to save the file in your computer. Line 4/4
		}

		
		//It measures the bytes received
		if(j==1){
			FILE* fb=fopen(benchmarkName,"a");				
			fprintf(fb, "File size(bytes), %d\n", auxBytes);
			fclose(fb);
		}


			t = clock() - t; 
			double time_taken = ((double)t)/CLOCKS_PER_SEC; // in second
			FILE* fb=fopen(benchmarkName,"a");//	
			fprintf(fb, "Execution time %s,  %f\n",thID, time_taken);//Response time of each cycle of each thread.
			fclose(fb);
			close(sock);
	
			
	}
	return 0;
}








