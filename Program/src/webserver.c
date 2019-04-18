/*
BSD 3-Clause License

Copyright (c) 2015-2019, Pablo Rodriguez Quesada, Diego, Giovanny
All rights reserved.
Based on Oscar Sanchez (oms1005@gmail.com) project

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/shm.h>

#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG   44
#define MAX_THREADS 1000
#if !defined(FORK) && !defined(FIFO) && !defined(THREADED) && !defined(PREFORK)
#error You must define FORK or FIFO or THREADED before compiling webserver use -D flag on gcc or TYPE= on make
#endif

#if defined(THREADED) || defined(PRETHREADED)
#include <pthread.h>
#endif

typedef struct web_args {
    int fd;
    int hit;
	int thread_id;  //or fork
} web_args_t;

struct {
	char *ext;
	char *filetype;
} extensions [] = { //Definition of the supported extensions
	{"gif", "image/gif" },  
	{"jpg", "image/jpeg"}, 
	{"jpeg","image/jpeg"},
	{"png", "image/png" },  
	{"zip", "image/zip" },  
	{"gz",  "image/gz"  },  
	{"tar", "image/tar" },  
	{"htm", "text/html" },  
	{"html","text/html" },  
	{"php", "text/php"  },  
	{"cgi", "text/cgi"  },  
	{"asp","text/asp"   },  
	{"jsp", "image/jsp" },  
	{"xml", "text/xml"  },  
	{"js","text/js"     },
    {"css","test/css"   }, 
	{"ico","image/ico"  },
	{"iso","image/iso"  },
	{"txt","text/txt"   },
	{0,0} };

//definition of paths
char * ROOT_FOLDER= "ROOT_DIR=";
char * PORT = "PORT=";
char * LOG_FILE = "LOGFILE=";


char * directory = NULL;
char * log_file_path = NULL;
char * port_str = NULL;
int socketfd;
#if defined(PREFORK) || defined(PRETHREADED)
sem_t *sem;
int shmid; 
int shmid_args; 
unsigned int numberOfForks=4; 
unsigned int semaphoreValue=1;
#endif
#if defined(THREADED) || defined(PRETHREADED)
pthread_t threads[MAX_THREADS];
char threads_status[MAX_THREADS] = { 0 };

size_t findIndex( const char a[], size_t size, int value )
{
    size_t index = 0;
    while ( index < size && a[index] != value ) ++index;
    return ( index == size ? -1 : index );
}

#endif

//This function is used to chop/cut strings
void slice_str(const char * str, char * buffer, size_t start, size_t end)
{
    size_t j = 0;
	size_t i;
    for ( i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}
//Funtion to register events in the log
void log_event(int type, char *s1, char *s2, int num)
{
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (type) {
	case ERROR: 
        (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid());
        break;
	case SORRY: 
		(void)sprintf(logbuffer, "<HTML><BODY><H1>Web Server Sorry: %s %s</H1></BODY></HTML>\r\n", s1, s2);
		(void)write(num,logbuffer,strlen(logbuffer));
		(void)sprintf(logbuffer,"SORRY: %s:%s",s1, s2); 
		break;
	case LOG: 
        (void)sprintf(logbuffer," INFO: %s:%s:%d",s1, s2,num);
        break;
	}	
	
	if((fd = open(log_file_path, O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer)); 
		(void)write(fd,"\n",1);      
		(void)close(fd);
	}
	fprintf( stderr, "%s",logbuffer );
	fprintf( stderr, "\n" );
	
	if(type == ERROR
	#ifdef FORK 
	|| type == SORRY
	#endif
	) exit(3);
	
	
}

//This function is used to control browser requests
void* web(void* input)
{
	int fd = ((struct web_args*)input)->fd;
	int hit = ((struct web_args*)input)->hit;
	int thread_id = ((struct web_args*)input)->thread_id;
	if(thread_id != -1){
		log_event(LOG, "Thread created", "" ,thread_id);
	}
	free(input);
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	#if !defined(THREADED) || !defined(PRETHREADED)
	static 
	#endif
	char buffer[BUFSIZE+1];

	ret = read(fd,buffer,BUFSIZE); 
	if(ret == 0 || ret == -1) {
		perror("Error on read request");
		log_event(SORRY,"failed to read browser request","",fd);
	}

	if(ret > 0 && ret < BUFSIZE)	
		buffer[ret]=0;	
	else buffer[0]=0;

	for(i=0;i<ret;i++)	
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i]='*';
	log_event(LOG,"request",buffer,hit);

	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) )
		log_event(SORRY,"Only simple GET operation supported",buffer,fd);

	for(i=4;i<BUFSIZE;i++) { 
		if(buffer[i] == ' ') { 
			buffer[i] = 0;
			break;
		}
	}

	for(j=0;j<i-1;j++) 	
		if(buffer[j] == '.' && buffer[j+1] == '.')
			log_event(SORRY,"Parent directory (..) path names not supported",buffer,fd);

	if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) 
		(void)strcpy(buffer,"GET /index.html");

	buflen=strlen(buffer);
	fstr = (char *)0;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr =extensions[i].filetype;
			break;
		}
	}
	if(fstr == 0) log_event(SORRY,"file extension type not supported",buffer,fd);

	if(( file_fd = open(&buffer[5],O_RDONLY)) == -1) 
		log_event(SORRY, "failed to open file",&buffer[5],fd);

	log_event(LOG,"SEND",&buffer[5],hit);

	(void)sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	(void)write(fd,buffer,strlen(buffer));

	while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
		(void)write(fd,buffer,ret);
	}
	(void)close(fd);
#ifdef FORK
	exit(1);
#endif
	
#if defined(THREADED) || defined(PRETHREADED)
	log_event(LOG, "Exiting thread", "" ,thread_id);
	if(thread_id != -1){
		threads_status[thread_id] = 0;
	}
#endif
}
#ifdef PREFORK
void waiting_fork(int *array ,  sem_t *sem, int i, web_args_t* args);
void working_fork(int *array ,  sem_t *sem, int i, web_args_t* args){
    sem_wait (sem);           /* P operation */
    array[i]= 0;              /* increment *p by 0, 1 or 2 based on i */
    printf ("+Child(%d) is in critical section with PID:(%d)\n", i, array[i]);
    sem_post (sem);           /* V operation */
    web((void*)(args+i));
    waiting_fork(array, sem, i, args);
}
void waiting_fork(int *array ,  sem_t *sem, int i, web_args_t* args){
    sem_wait (sem);           /* P operation */
    array[i]= getpid();              /* increment *p by 0, 1 or 2 based on i */
    printf ("*Child(%d) is in critical section with PID:(%d)\n", i, array[i]);
    sem_post (sem);           /* V operation */
    printf ("Waiting"); 
    if (kill (getpid(), SIGSTOP) == -1) {
                    perror ("kill of child failed"); exit (-1);
    }  
	printf("Awoke pid: %d\n",getpid());
    working_fork(array, sem, i, args);
}
#endif
void intHandler(int a) {
    perror ("SIGINT received");
	log_event(LOG, "SIGINT received", "" ,a);
	(void)close(socketfd);
    exit(1);	
}
//Function to read the configuration file
int check_phrase(const char * line, const char * phrase, int len_line, char * result){
	if (!strncmp(line,phrase,strlen(phrase)-1 )){
			slice_str(line, result,strlen(phrase),len_line-1);
			result[len_line-strlen(phrase)-1]= '\0';
			return 1;
		} 
	return 0;
}

int main(int argc, char **argv)
{
	int i, port, pid, listenfd, hit;
	socklen_t socket_length;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;
	#if defined(PREFORK)
	key_t key = ftok("shmfile",65); 
	// shmget returns an identifier in shmid 
    shmid = shmget(key,sizeof(int)*numberOfForks,0666|IPC_CREAT);
	int *IPCflags = (int*) shmat(shmid,(void*)0,0); 

	key_t key_args = ftok("shmfile2",65); 
	// shmget returns an identifier in shmid 
    shmid_args = shmget(key_args,sizeof(web_args_t)*numberOfForks,0666|IPC_CREAT);
	web_args_t *IPCwebargs = (web_args_t*) shmat(shmid,(void*)0,0); 
	
	//Semaphore
	sem = sem_open ("pSem", O_CREAT | O_EXCL, 0644, semaphoreValue); 
	for (i = 0; i < numberOfForks; i++){
		//printf("Creating new fork from %d\n",numberOfForks);
        pid = fork();
		//printf("PID: %d\n", pid);
        if (pid < 0) {
        	/* check for error      */
            sem_unlink ("pSem");   
            sem_close(sem);  
            /* unlink prevents the semaphore existing forever */
            /* if a crash occurs during the execution         */
            printf ("Fork error.\n");
        }
         else if (pid == 0){
		 	//printf("Hello from new Fork!\n");
		 	goto start_child;
            break;  
		 }
        
    }
	#endif
	if(  argc < 1  || (argc == 2 && !strcmp(argv[1], "-h")) || argc > 1) {
		printf("Use configuration file at /etc/webserver\nOnly Supports:");
		for(i=0;extensions[i].ext != 0;i++)
			printf(" %s",extensions[i].ext);
			printf("\n\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n");
		exit(0);
	}



	// Read configuration file
	//------------------------------------
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
	fp = fopen("/etc/webserver/config.conf", "r");
    if (fp == NULL){
		printf("Couldn't  open configuration file /etc/webserver/config.conf\nCheck permissions:\n$ ls -la /etc/webserver/config.conf\n");
        exit(EXIT_FAILURE);
	}
	directory = malloc(sizeof(char)*BUFSIZE*2);
	port_str = malloc(sizeof(char)*BUFSIZE*2);
	log_file_path=malloc(sizeof(char)*BUFSIZE*2);
    while ((read = getline(&line, &len, fp)) != -1) {
		// Get ROOT Folder
		if (check_phrase(line,ROOT_FOLDER,read,directory)){
		}
		//Get PORT
		else if (check_phrase(line,PORT,read,port_str)){
			port = atoi(port_str);
		}
		//Get Logfile
		else if (check_phrase(line,LOG_FILE,read,log_file_path)){
		}
    }
	//Close file
    fclose(fp);
    if (line)
        free(line);
	if (!log_file_path){
		log_file_path = "/var/log/webserver.log";
	}

	//Calls to log messages
	log_event(LOG, "Directory", directory ,0);
	log_event(LOG, "Port", port_str ,0);
	log_event(LOG, "LOG Folder", log_file_path, 0);
	//------------------------------------

	if( !strncmp(directory,"/"   ,2 ) || !strncmp(directory,"/etc", 5 ) ||
	    !strncmp(directory,"/bin",5 ) || !strncmp(directory,"/lib", 5 ) ||
	    !strncmp(directory,"/tmp",5 ) || !strncmp(directory,"/usr", 5 ) ||
	    !strncmp(directory,"/dev",5 ) || !strncmp(directory,"/sbin",6) ){
		printf("ERROR: Bad top directory %s, see server -?\n",directory);
		exit(3);
	}
	if(chdir(directory) == -1){ 
		printf("Can't Change to directory %s: \n",directory);
		perror("ERROR "); 
		exit(4);
	}


	signal(SIGCLD, SIG_IGN); 
	signal(SIGHUP, SIG_IGN); 
	signal(SIGINT, intHandler); 
	signal(SIGKILL, intHandler);
	signal(SIGPIPE, SIG_IGN);
	for(i=0;i<32;i++)
		(void)close(i);	
	(void)setpgrp();	




	log_event(LOG,"HTTP server starting",port_str,getpid());

	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		log_event(ERROR, "system call","socket",0);
	if(port < 0 || port >65000)
		log_event(ERROR,"Invalid port number try [1,65000]",port_str,0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if (bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0){
		perror("Error on bind");
		log_event(ERROR,"system call","bind",0);
	}
	if( listen(listenfd,64) <0)
		log_event(ERROR,"system call","listen",0);

	for(hit=1; ;hit++) {
		socket_length = (socklen_t) sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &socket_length)) < 0)
			log_event(ERROR,"system call","accept",0);
		int status;
		int enable = 1;
		if (status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    		log_event(ERROR, "Setsocketopts", "Error code" , status);
		if (status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    		log_event(ERROR, "Setsocketopts", "Error code" , status);
		struct web_args*  request_args = (struct web_args *)malloc(sizeof(struct web_args));
		request_args->fd = socketfd;
		request_args->hit = hit;
		request_args->thread_id = -1;
		signal(SIGPIPE, SIG_IGN);
#ifdef PREFORK
		printf("Entered prefork\n");
		int attended = 0;
		while(!attended){
			printf("Checking\n");
			int f;
			for(f=0;f<numberOfForks;f++){
				if (IPCflags[f]!=0 ) {
					memcpy(IPCwebargs + f, request_args, sizeof(web_args_t)); 
					printf("Waking up PID %d\n",IPCflags[f]);
					kill (IPCflags[f], SIGCONT);
					attended = 1;
					break;
				}
			}
		}
		//web(request_args);
		continue;
start_child:
		waiting_fork(IPCflags, sem, i, IPCwebargs);
		exit(0);
#endif
// Multiple forks
#ifdef FORK
		if((pid = fork()) < 0) {
			log_event(ERROR,"system call","fork",0);
		}
		else {
			if(pid == 0) {
				(void)close(listenfd);
				web(request_args);
				exit(0);
			} else {
				(void)close(socketfd);
			}
		}
#endif

#ifdef THREADED
	request_args->thread_id = findIndex(threads_status, MAX_THREADS , 0);
	if (request_args->thread_id == -1){
		log_event(ERROR, "MAX threads ", "" ,0);
		(void)close(socketfd);
	}
	pthread_create(&threads[request_args->thread_id], NULL, web, (void *)request_args);
	threads_status[request_args->thread_id] = 1;
	//pthread_join(&threads[request_args->thread_id], NULL);
#endif

//Serial Mode
#ifdef FIFO
		web(request_args);
#endif
	}
}
