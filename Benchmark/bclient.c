#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdlib.h> 
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h> 
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include <string.h>



int ReadHttpStatus(int sock){
    char c;
     
    char buff[1024]="",*ptr=buff+1;
    int bytes_received, status;
    printf("Begin Response ..\n");
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

    //printf("%s\n",ptr);
    printf("status=%d\n",status);
    //printf("End Response ..\n");
    return (bytes_received>0)?status:0;

}

//the only filed that it parsed is 'Content-Length' 
int ParseHeader(int sock){
    char c;
    char buff[1024]="",*ptr=buff+4;
    int bytes_received, status;
    //printf("Begin HEADER ..\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("Parse Header");
            exit(1);
        }

        if(
            (ptr[-3]=='\r')  && (ptr[-2]=='\n' ) &&
            (ptr[-1]=='\r')  && (*ptr=='\n' )
        ) break;
        ptr++;
    }

    *ptr=0;
    ptr=buff+4;
    //printf("%s",ptr);

    if(bytes_received){
        ptr=strstr(ptr,"Content-Length:");
        if(ptr){
            sscanf(ptr,"%*s %d",&bytes_received);

        }else
            bytes_received=-1; //unknown size

       //printf("Content-Length: %d\n",bytes_received);
    }
   // printf("End HEADER ..\n");
    return  bytes_received ;

}

typedef struct thread_arg {
	long int id;
	int  nthread;
	int  ncycle;
	char domain[30];
	char path[30];
}thread_arg;







void* downloadFile(thread_arg * arg){
	
	char domain[30];
	char path[30];

	strcpy(domain, arg->domain);
	strcpy(path,arg->path);

    int sock, bytes_received;  
    char send_data[1024],recv_data[1024], *p;
    struct sockaddr_in server_addr;
    struct hostent *he;


    he = gethostbyname(domain);
    if (he == NULL){
       herror("gethostbyname");
       exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0))== -1){
       perror("Socket");
       exit(1);
    }
    server_addr.sin_family = AF_INET;     
    server_addr.sin_port = htons(80);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(server_addr.sin_zero),8); 

    printf("Connecting ...\n");
    if (connect(sock, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
       perror("Connect");
       exit(1); 
    }


    snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);

    if(send(sock, send_data, strlen(send_data), 0)==-1){
        perror("send");
        exit(2); 
    }
    //printf("Data sent.\n");  

    //fp=fopen("received_file","wb");
    //printf("Recieving data...\n\n");

    int contentlengh;
	time_t beginR; //Total Execution time
	time_t endR;
	time(&beginR);
    
    if(ReadHttpStatus(sock) && (contentlengh=ParseHeader(sock))){

	 time(&endR);
	 printf("Response time %f\n", difftime(endR, beginR));
	int bytes=0;
        
        FILE* fd=fopen(path,"wb");
        printf("Saving data...");

        while(bytes_received = recv(sock, recv_data, 1024, 0)){
            if(bytes_received==-1){
                perror("recieve");
                exit(3);
            }
            fwrite(recv_data,1,bytes_received,fd);
            bytes+=bytes_received;
            //printf("Bytes recieved: %d\n",bytes);
            if(bytes==contentlengh)
            break;
        }
        fclose(fd);
    }



    close(sock);
    printf("\n\nDone.\n\n");

}






int main(int argc, char *argv[]){
 
  // # bclient <machine> <port> <file> <N-threads> <N-cycles>
	 time_t begin; //Total Execution time
	 time_t end;
     
	if( argc ==6){
		char machine[20];
		char file[20];
		int port=0;
		int threads=0;
		int cycles=0;
		
		 strcpy(machine, argv[1]);
		 strcpy(file, argv[3]);
			
		
		threads=atoi(argv[4]);
		port=atoi(argv[2]);
		cycles=atoi(argv[5]);
		
		
		
		printf("Machine: %s \n", machine); // 
		printf("file: %s \n", file); // 
		printf("Port: %d \n", port); // 
		printf("Threads:  %d \n", threads); // 
		printf("Cycles: %d \n", cycles); // 
		//downloadFile();
		printf("\nURL: %s/%s\n", machine,file);
		

	 pthread_t thread_id[threads]; 
	  struct thread_arg arg[threads];
	  
	  
    time(&begin);
    time_t test = clock();
    
	  for(int i=0;i<threads;i++)
    {
		 arg[i].nthread = i+1;
		 arg[i].ncycle=cycles;
		 strcpy(arg[i].domain, machine);
		  strcpy(arg[i].path,file);
		pthread_create(&thread_id[i], NULL, downloadFile, &arg[i]); 
		 arg[i].id = thread_id[i];
		// printf("In main \nthread id = %ld\n", (long)  arg[i].id );
		//printf("In main nthread id = %d\n",  arg.nthread );
		
	} 
	
	 for(int i=0;i<threads;i++)
    {
		pthread_join(thread_id[i], NULL); 
		
	}
	   
   time(&end);
	printf("Total execution time %f\n", difftime(end, begin));
	pthread_exit(NULL); 
		
	}
	return 0;
}


