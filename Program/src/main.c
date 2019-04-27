#include "webserver.h"


//######### VARIABLES ################

//definition of paths
char * ROOT_FOLDER= "ROOT_DIR=";
char * PORT = "PORT=";
char * LOG_FILE = "LOGFILE=";
char * NUMBER_OF_FORKS = "NUMBER_OF_FORKS=";


char * directory = NULL;
char * log_file_path = NULL;
char * port_str = NULL;
char * number_of_forks_str = NULL;
int socketfd;
unsigned int numberOfForks = 4; 

//If defined is a command for the compiler so that it ignores this code if the parameter is not defined
#if defined(THREADED) || defined(PRETHREADED)
//Definition of variables specific to THREADED and PRETHREADED versions of the server
bool initial_call_prethreaded = true;
pthread_t threads[MAX_THREADS];

void* t_arguments[MAX_THREADS];

char threads_status[MAX_THREADS] = { 0 };

sigset_t signal_set;
int sig_int;

#endif



//############## FUNCTIONS ##############
//Finds the position of the first value in the list that has the input value.
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
	//open file
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
	//file descriptor definition and assignation
	int fd = ((struct web_args*)input)->fd;
	int hit = ((struct web_args*)input)->hit;
	int thread_id = ((struct web_args*)input)->thread_id;

	#ifndef PREFORK 
	if(thread_id != -1){
		log_event(LOG, "Thread created", "" ,thread_id);
	}
	#endif

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
	log_event(LOG, "Exiting thread", "" ,thread_id); //Exit thread status register
	if(thread_id != -1){
		threads_status[thread_id] = 0;
	}
#endif
}
//The following function is the function assigned to the threads. This handles the waiting of the thread and the execution of web
#if defined(PRETHREADED)
void* t_controller(void* t_args){
	int temp = 0;
	temp = (((struct web_args*)t_args))->thread_id; //assignation of arguments
	start: ;
	sigwait( &signal_set, &sig_int);


	temp = (((struct web_args*)t_args))->thread_id; //Reassignation of arguments in case they where changed during waiting time
	web(t_args);
	threads_status[temp] = 0; //free from work, ready for more
	goto start;
}
#endif

//Signal handler
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
//Main process that creates and assigns work
int main(int argc, char **argv)
{
	int i, port, pid, listenfd, hit;
	socklen_t socket_length;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;
	//printf("DEBUG: %s %s:%d\n", __func__, __FILE__, __LINE__);

	#if defined(THREADED) || defined(PRETHREADED)
	#ifdef MY_PTHREAD_LIB
	#warning "MyPthread library included!"
	my_pthread_setsched(SCHEDULER);
	#endif 
	#endif

	#if defined(PRETHREADED)
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGCONT); 
	sigprocmask(SIG_BLOCK, &signal_set, NULL);
	#endif
	//printf("DEBUG: %s %s:%d\n", __func__, __FILE__, __LINE__);


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
	number_of_forks_str=malloc(sizeof(char)*BUFSIZE*2);
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
		//Get Logfile
		else if (check_phrase(line,NUMBER_OF_FORKS,read,number_of_forks_str)){
			numberOfForks = atoi(number_of_forks_str);
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
	log_event(LOG, "Configuration from", "/etc/webserver/config.conf" ,1);
	log_event(LOG, "Directory", directory ,0);
	log_event(LOG, "Port", "0.0.0.0" ,port);
	log_event(LOG, "LOG Folder", log_file_path, 0);
	#ifdef PREFORK
	log_event(LOG, "Number of Forks", "", numberOfForks);
	#endif
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
	//signals handling
	signal(SIGCLD, SIG_IGN); 
	signal(SIGHUP, SIG_IGN); 
	signal(SIGINT, intHandler); 
	signal(SIGKILL, intHandler);
	signal(SIGPIPE, SIG_IGN);

	// for(i=0;i<32;i++)
	// 	(void)close(i);	
	
	(void)setpgrp();	




	log_event(LOG,"HTTP server starting",port_str,getpid());
	
	//creates the required amount of memory spaces for the arguments to be passed to the threads (only on the first request)
	#if defined (PRETHREADED)
	if (initial_call_prethreaded) {
		
		for (i = 0; i < MAX_THREADS;i++){
			struct web_args*  requested_args = (struct web_args *)malloc(sizeof(struct web_args));
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
#ifndef PREFORK

	for(hit=1; ;hit++) {
		socket_length = (socklen_t) sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &socket_length)) < 0)
			log_event(ERROR,"system call","accept",0);
		#if defined(THREADED) || defined(PRETHREADED)
		#ifdef MY_PTHREAD_LIB
		raise(SIGVTALRM);
		#endif
		#endif
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
		
#endif
#ifdef PREFORK
	int h, m;


    /* Fork you some child processes. */
    for (h = 0; h < numberOfForks; h++) {
	pid = fork();
	if (pid == -1) {
	    log_event(ERROR, "Couldn't fork","",0);
		perror("Couldn't fork");
		exit(1);
	}

	if (pid == 0) { // We're in the child ...
	    for (;;) { // Run forever ...
		/* Necessary initialization for accept(2) */
		//socket_length = sizeof client;

		/* Blocks! */
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &socket_length)) < 0)
			log_event(ERROR,"system call","accept",0);
		
		
		int status;
		int enable = 1;
		if (status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    		log_event(ERROR, "Setsocketopts", "Error code" , status);
		if (status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
    		log_event(ERROR, "Setsocketopts", "Error code" , status);
		
		if (status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    		log_event(ERROR, "Setsocketopts", "Error code" , status);
		 
		 
		web_args_t* request_args = (struct web_args *)malloc(sizeof(struct web_args));	
		request_args->fd = socketfd;
		request_args->hit = hit;
		request_args->thread_id = -1;
		signal(SIGPIPE, SIG_IGN);
		log_event(LOG,"New Request\n","",0);
		/* write 10 lines of text into the file stream*/
		log_event(LOG,"CPI", " PID", getpid());
		
		web(request_args);
		/* Clean up the client socket */
	///	close(clientfd);
	(void)close(socketfd);
	    }
	}
    }

    /* Sit back and wait for all child processes to exit */
    while (waitpid(-1, NULL, 0) > 0);

    /* Close up our socket */
   // close(sockfd);
   (void)close(socketfd);

    return 0;

#endif

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
	request_args->thread_id = findIndex(threads_status, MAX_THREADS , 0); //Finds non working thread
	if (request_args->thread_id == -1){
		log_event(ERROR, "MAX threads ", "" ,0);
		(void)close(socketfd);
	}
	pthread_create(&threads[request_args->thread_id], NULL, web, (void *)request_args); //creates thread to handle the work
	threads_status[request_args->thread_id] = 1; //updates status
	#ifdef MY_PTHREAD_LIB
	raise(SIGVTALRM); 
	time_t start = time(NULL);
    while (1){
        time_t now = time(NULL);
        if(now >= start + 1) break;
    }
	#endif
	//pthread_join(&threads[request_args->thread_id], NULL);
#endif

#ifdef PRETHREADED
	if (initial_call_prethreaded == false) {
		goto end;
	}
	int temp = 0;
	//Creates the required amount of threads and asign them their respective arguments.
	for (i = 0; i < MAX_THREADS; i++){
		struct web_args*  requested_args = (struct web_args *)malloc(sizeof(struct web_args)); //adquires memory for arguments
		requested_args -> thread_id = i; //sets id
		t_arguments[i] = (void *)requested_args; //assignates memory
		pthread_create(&threads[i], NULL, t_controller, t_arguments[i]);  //creates thread and assigns pointer to arguments
		threads_status[i] = 0; //updates status to "free"

	}
		
	initial_call_prethreaded = false; //No need for more creations

	end: ;

	temp = findIndex(threads_status, MAX_THREADS , 0);
	if (temp == -1){

		goto end;
	}

	(((struct web_args*)t_arguments[temp]))->fd = request_args -> fd;
	(((struct web_args*)t_arguments[temp]))->hit = request_args -> hit;

	threads_status[temp] = 1;

	pthread_kill(threads[temp], SIGCONT); //Tells the thread to be used to begin work


	#endif

//Serial Mode
#ifdef FIFO
		web(request_args);
#endif

#ifndef PREFORK
	}
#endif
}
