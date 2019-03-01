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

#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG   44

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },  
	{"jpg", "image/jpeg"}, 
	{"jpeg","image/jpeg"},
	{"png", "image/png" },  
	{"zip", "image/zip" },  
	{"gz",  "image/gz"  },  
	{"tar", "image/tar" },  
	{"htm", "text/html" },  
	{"html","text/html" },  
	{"php", "text/php" },  
	{"cgi", "text/cgi"  },  
	{"asp","text/asp"   },  
	{"jsp", "image/jsp" },  
	{"xml", "text/xml"  },  
	{"js","text/js"     },
    {"css","test/css"   }, 
	{"iso","image/iso"   },
	{0,0} };



char * ROOT_FOLDER= "ROOT_DIR";
char * PORT = "PORT";
char * LOG_FILE = "LOGFILE";


char * directory = NULL;
char * log_file_path = NULL;
char * port_str = NULL;

void slice_str(const char * str, char * buffer, size_t start, size_t end)
{
    size_t j = 0;
	size_t i;
    for ( i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}

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
	printf("%s",logbuffer);
	if(type == ERROR || type == SORRY) exit(3);
}

void web(int fd, int hit)
{
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	static char buffer[BUFSIZE+1];

	ret =read(fd,buffer,BUFSIZE); 
	if(ret == 0 || ret == -1) {
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
#ifdef LINUX
	sleep(1);
#endif
	exit(1);
}

void intHandler(int a) {
    //keepRunning = false;
    exit(1);	
}
int check_phrase(const char * line, const char * phrase, int len_line, char * result){
	if (!strncmp(line,phrase,strlen(phrase)-1 )){
			printf("%s", line);
			slice_str(line, result,strlen(phrase)+1,len_line-2);
			directory[len_line-strlen(phrase)-2]= '\0';
			return 1;
		} 
	return 0;
}
int main(int argc, char **argv)
{
    struct sigaction act;
    act.sa_handler = intHandler;
    sigaction(SIGINT, &act, NULL);

	int i, port, pid, listenfd, socketfd, hit;
	socklen_t socket_length;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;

	if(  argc < 1  || (argc == 2 && !strcmp(argv[1], "-h")) || argc > 1) {
		printf("Use configuration file at /etc/webserver\nOnly Supports:");
		for(i=0;extensions[i].ext != 0;i++)
			printf(" %s",extensions[i].ext);
			printf("\n\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n");
		exit(0);
	}
	// Read configuration file
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
			printf("Directory: %s\n", directory);
		}
		//Get PORT
		else if (check_phrase(line,PORT,read,port_str)){
			port = atoi(port_str);
			printf("Port: %d\n", port);
		}
		//Get Logfile
		else if (check_phrase(line,LOG_FILE,read,log_file_path)){
			printf("Log File: %s\n", log_file_path);
		}
    }

	//Close file
    fclose(fp);
    if (line)
        free(line);
	if (!log_file_path){
		log_file_path = "/var/log/webserver.log";
	}
	if( !strncmp(directory,"/"   ,2 ) || !strncmp(directory,"/etc", 5 ) ||
	    !strncmp(directory,"/bin",5 ) || !strncmp(directory,"/lib", 5 ) ||
	    !strncmp(directory,"/tmp",5 ) || !strncmp(directory,"/usr", 5 ) ||
	    !strncmp(directory,"/dev",5 ) || !strncmp(directory,"/sbin",6) ){
		printf("ERROR: Bad top directory %s, see server -?\n",directory);
		exit(3);
	}
	printf("%s", directory);
	if(chdir(directory) == -1){ 
		printf("Can't Change to directory %s: \n",directory);
		perror("ERROR "); 
		exit(4);
	}


	(void)signal(SIGCLD, SIG_IGN); 
	(void)signal(SIGHUP, SIG_IGN); 
	for(i=0;i<32;i++)
		(void)close(i);	
	(void)setpgrp();	

	log_event(LOG,"http server starting",argv[1],getpid());

	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		log_event(ERROR, "system call","socket",0);
	if(port < 0 || port >60000)
		log_event(ERROR,"Invalid port number try [1,60000]",argv[1],0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		log_event(ERROR,"system call","bind",0);
	if( listen(listenfd,64) <0)
		log_event(ERROR,"system call","listen",0);

	for(hit=1; ;hit++) {
		socket_length = (socklen_t) sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &socket_length)) < 0)
			log_event(ERROR,"system call","accept",0);

		if((pid = fork()) < 0) {
			log_event(ERROR,"system call","fork",0);
		}
		else {
			if(pid == 0) {
				(void)close(listenfd);
				web(socketfd,hit);
			} else {
				(void)close(socketfd);
			}
		}
	}
}
