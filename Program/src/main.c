#include "webserver.h"


//######### VARIABLES ################

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
bool initial_call_prethreaded = true;
pthread_t threads[MAX_THREADS];

void* t_arguments[MAX_THREADS];

char threads_status[MAX_THREADS] = { 0 };

sigset_t signal_set;
int sig;

#endif



//############## FUNCTIONS ##############

size_t findIndex( const char a[], size_t size, int value )
{
    size_t index = 0;
    while ( index < size && a[index] != value ) ++index;
    return ( index == size ? -1 : index );
}

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
	#ifndef PREFORK
	free(input);
	#endif
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	#if !defined(THREADED) || !defined(PRETHREADED)
	static 
	#endif
	char buffer[BUFSIZE+1];
	ret = read(fd,buffer,BUFSIZE); 
	if(ret == 0 || ret == -1) {
		printf("Error reading :%d\n",fd);
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

#if defined(PRETHREADED)
void* t_controller(void* t_args){
	
	//printf("Leroy Jenkins!!!\n");
	int temp = 0;
	//int dummy = 0;
	temp = (((struct web_args*)t_args))->thread_id;

	start: ;
	//printf("Leroy Jenkins!!! - 2\n");
	sigwait( &signal_set, &sig);
	//printf("Leroy Jenkins!!! - 3\n");
	//t_args = t_arguments[temp];

	//printf("Leroy Jenkins!!!\n");

	temp = (((struct web_args*)t_args))->thread_id;

	//printf("Temp in t_controller: %i \n",temp);
	//printf("t_args in t_controller before while: %i \n",t_args);
	//while (threads_status[temp] == 0){ //Is the thread to be used?, if not just keep waiting for job.
		//printf("Goddamnit Leroy! %i\n", temp);
		//dummy +=1;
		//dummy = 0;
		//wait(1);
	//}

	//t_args = t_arguments[temp];

	//printf("t_args in t_controller after while: %i \n",t_args);
	//threads_status[temp] == 1;
	//printf("Before the fall, there is always peace\n");

	web(t_args);

	//printf("I'm dead\n");
	threads_status[temp] == 0; //free from work, ready for more
	//printf("Before own stop (is changing because of having SIGSTOP?)\n");
	//printf("Before own stop) - threads[temp] (thread id in system): %i \n",threads[temp]);
	//printf("This is pthread_Kill SIGSTOP return value: (t_controller) %i \n", pthread_kill(threads[temp], SIGSTOP));
	//pthread_kill(pthread_self(), SIGSTOP);
	
	//printf("At leats i have chicken \n");

	goto start;
	//t_controller(t_arguments[temp]);
}
#endif

#ifdef PREFORK
void waiting_fork(int *array ,  sem_t *sem, int i, web_args_t* args);
void working_fork(int *flags ,  sem_t *sem, int i, web_args_t* args){
    sem_wait (sem);           /* P operation */
	web_args_t* request_ptr = (args+i);
	printf("Awoke pid: %d\n",getpid());
	web_args_t request = *request_ptr;
	printf("File descriptor : %d, hit %d\n",request.fd, request.hit);
    flags[i]= 0;              /* increment *p by 0, 1 or 2 based on i */
    printf ("+Child(%d) is in critical section with PID:(%d)\n", i, flags[i]);
    sem_post (sem);           /* V operation */
    web(&request);
    waiting_fork(flags, sem, i, args);
}
void waiting_fork(int *array ,  sem_t *sem, int i, web_args_t* args){
    sem_wait (sem);           /* P operation */
    array[i]= getpid();              /* increment *p by 0, 1 or 2 based on i */
    printf ("*Child(%d) is in critical section with PID:(%d)\n", i, array[i]);
    sem_post (sem);           /* V operation */
    printf ("Waiting \n"); 
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
	#if defined(PREFORK)
	sem_unlink ("pSem");   
    sem_close(sem);  
	#endif
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

	#if defined(PRETHREADED)
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGCONT); 
	sigprocmask(SIG_BLOCK, &signal_set, NULL);
	#endif

	#if defined(PREFORK)
	//close semaphore if left open
	sem_unlink ("pSem");   
    sem_close(sem);  
	key_t key = ftok("shmfile",65); 
	// shmget returns an identifier in shmid 
    shmid = shmget(key,sizeof(int)*numberOfForks,0666|IPC_CREAT);
	int *IPCflags = (int*) shmat(shmid,(void*)0,0); 

	key_t key_args = ftok("shmfile2",66); 
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

	#if defined (PRETHREADED)
	if (initial_call_prethreaded) {
		
		for (i = 0; i < MAX_THREADS;i++){
			struct web_args*  requested_args = (struct web_args *)malloc(sizeof(struct web_args));
			//printf("%s","Arguments Created\n");
			t_arguments[i] = (void *)requested_args;
		}
	}
	#endif

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
		log_event(LOG,"New Request\n","",0);
		int asda =0;
#ifdef PREFORK
		int attended = 0;
		while(!attended){
			int f;
			for(f=0;f<numberOfForks;f++){
				int child_pid = IPCflags[f];
				if (child_pid!=0 ) {
					memcpy(IPCwebargs + f, request_args, sizeof(web_args_t)); 
					(void)close(socketfd);
					kill (child_pid, SIGCONT);
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

#ifdef PRETHREADED
	//do here the static things
	////printf("%s","Before Thread Creation Process\n");
	if (initial_call_prethreaded == false) {
		//printf("%s","After initial call \n");
		goto end;
	}
	int temp = 0;

	for (i = 0; i < MAX_THREADS; i++){
		struct web_args*  requested_args = (struct web_args *)malloc(sizeof(struct web_args));
		requested_args -> thread_id = i;
		t_arguments[i] = (void *)requested_args;
		pthread_create(&threads[i], NULL, t_controller, t_arguments[i]);
		threads_status[i] = 0;

	}

	initial_call_prethreaded = false;

	end: ;

	temp = findIndex(threads_status, MAX_THREADS , 0);
	if (temp == -1){

		goto end;
	}

	(((struct web_args*)t_arguments[temp]))->fd = request_args -> fd;
	(((struct web_args*)t_arguments[temp]))->hit = request_args -> hit;

	threads_status[temp] = 1;

	pthread_kill(threads[temp], SIGCONT);


	#endif

//Serial Mode
#ifdef FIFO
		web(request_args);
#endif
	}
}
