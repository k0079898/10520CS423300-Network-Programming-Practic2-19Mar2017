#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

# define MAXLINE  4096

void(*signal(int signo,void(*func)(int)))(int);
typedef void Sigfunc(int);

Sigfunc *signal(int signo, Sigfunc *func){
	struct sigaction act,oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if(signo == SIGALRM){
		#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
		#endif	
	}else{
		#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;
		#endif	
	}
	if (sigaction(signo, &act, &oact)<0)
		return(SIG_ERR);
	return (oact.sa_handler);
}
void sig_chld(int signo){
	pid_t pid;
	int stat;
	while(pid == 0) {
		printf("Zombie child\n");
	}
	/*
	while((pid = waitpid(-1,&stat,WUNTRACED))>0){
		
		printf("child %d terminated \n",pid);
	}
	*/
	wait(&stat);
}
void str_echo(int sockfd){
	ssize_t n;
	int PID = setsid();
	int read_bytes;
	char buf[MAXLINE];
	again:
		while((read_bytes = read(sockfd,buf,MAXLINE))>0) {
			buf[read_bytes] = '\0';
			printf("PID %d client: %s", PID, buf);
			write(sockfd,buf,strlen(buf));
                }
		if(n<0 && errno == EINTR)
			goto again;
		else if (n<0)
			printf("str_echo:read error");
}
int main(int argc,char **argv){
	int listenfd,connfd;
	pid_t childpid;
	socklen_t clilen;
	char * ip;
	struct sockaddr_in cliaddr,servaddr;
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(9877);
	bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	listen(listenfd,1024);
	signal(SIGCHLD,sig_chld);
	puts("Waiting for new connection....");
	for(;;){
		clilen = sizeof(cliaddr);
		if ((connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen))< 0){
			if(errno == EINTR)
				continue;
			else
				printf("Accept failed");		
		}
		ip = inet_ntoa(cliaddr.sin_addr);
		printf("Client is from: %s port:%d \n",ip,cliaddr.sin_port);
		if((childpid = fork()) == 0){
			close(listenfd);
			str_echo(connfd);
			exit(0);
		}
	}
	close(connfd);
}
