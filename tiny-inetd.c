#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t interrupted;
volatile sig_atomic_t child_done;

void sighandler(int sig)
{
	switch(sig)
	{
	case SIGINT:
		interrupted=1;
		break;
	case SIGCHLD:
		child_done=1;
		break;
	}
	signal(sig,sighandler);
}

const char *myname;

int main(int argc,char **argv)
{
	int lis;	/*Listener socket*/
	unsigned long temp_ul;
	unsigned short port;
	char *endptr;
	int ret;
	struct sockaddr_in addr={0};

	myname=argv[0];
	argc--; argv++;

	if(argc < 2)
	{
		fprintf(stderr,"Usage: %s port cmd args...\n",myname);
		exit(0);
	}

	temp_ul=strtoul(argv[0],&endptr,10);
	if(temp_ul==0 || temp_ul > (unsigned short)-1 || *endptr!='\0')
	{
		fprintf(stderr,"Usage: %s port cmd args...\n",myname);
		exit(EXIT_FAILURE);
	}
	port=temp_ul;
	argc--; argv++;

	/*Set up socket*/
	lis=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(lis==-1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=INADDR_ANY;
	if(bind(lis,(struct sockaddr *)&addr,sizeof addr) == -1)
	{
		perror("bind");
		close(lis);
		exit(EXIT_FAILURE);
	}
	if(listen(lis,128) == -1)
	{
		perror("listen");
		close(lis);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"Listening on port %hu\n",port);

	signal(SIGINT,sighandler);
	signal(SIGCHLD,sighandler);

	while(1)
	{
		struct sockaddr_in remote;
		socklen_t len=sizeof remote;
		int sock;

		sock=accept(lis,(struct sockaddr *)&remote,&len);

		fprintf(stderr,"Got connection from %s\n",inet_ntoa(remote.sin_addr));

		if(sock >= 0)
		{
			ret=fork();
			if(ret < 0)
			{
				/*fork() failed*/
				perror("fork");
				exit(EXIT_FAILURE);
			}
			else if(ret==0)
			{
				/*child*/
				if(dup2(sock,0)==-1 || dup2(sock,1)==-1)
				{
					perror("dup2");
					exit(EXIT_FAILURE);
				}
				execvp(argv[0],argv);
				/*If we get here, exec failed*/
				perror("exec");
				exit(EXIT_FAILURE);
				/*If we get here, the world is about to end*/
			}
			else
			{
				/*parent*/
				close(sock);
			}
		}
#if 0	/*XXX HACK Darwin seems to mishandle signals while accept() is blocking*/
		else if(errno==EINTR)
		{
#endif
			if(interrupted)
			{
				fprintf(stderr,"Interrupted, cleaning up\n");
				break;	/*out of loop*/
			}
			if(child_done)
			{
				/*Clean up children*/
				child_done=0;
				while(waitpid(-1,&ret,WNOHANG) > 0)
				{
					fprintf(stderr,"Child process finished\n");
				}
			}
#if 0	/*XXX HACK Darwin seems to mishandle signals while accept() is blocking*/
		}
#endif
	}

	/*We only get here on normal exit - if a syscall fails, we bail out
	    immediately.
	*/
	close(lis);
	fprintf(stderr,"Waiting for children (interrupt again to stop immediately)\n");
	while(wait(&ret) > 0)	/*EINTR or ECHILD means stop*/
		;
	exit(0);
}
