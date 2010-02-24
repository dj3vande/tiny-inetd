#include <unistd.h>

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

	myname=argv[0];
	argc--; argv++;

	if(argc < 2)
	{
		fprintf(stderr,"Usage: %s port cmd args...\n",myname);
		exit(0);
	}

	temp_ul=strtoul(argv[0],10,&endptr);
	if(temp_ul==0 || temp_ul > (unsigned short)-1 || *endptr!='\0')
	{
		fprintf(stderr,"Usage: %s port cmd args...\n",myname);
		exit(EXIT_FAILURE);
	}
	port=temp_ul;
	argc--; argv++;

	/*Set up socket*/
	fprintf(stderr,"Listening on port %hu\n",port);

	while(1)
	{
		struct sockaddr_in remote;
		socklen_t len=sizeof remote;
		int sock;

		sock=accept(lis,(struct sockaddr *)&remote,&len);

		/*Log that we've accepted a connection?*/

		if(ret >= 0)
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
		else if(errno==EINTR)
		{
			if(interrupted)
			{
				fprintf(stderr,"Interrupted, cleaning up\n");
				break;	/*out of loop*/
			}
			if(child_waiting)
			{
				/*Clean up children*/
				child_waiting=0;
				while(waitpid(-1,WNOHANG,&ret) > 0)
					;
			}
		}
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
