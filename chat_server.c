#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<netdb.h>
#include<signal.h>
#include<unistd.h>
#include<error.h>
#include<arpa/inet.h>
#include<sys/un.h>
#include<errno.h>
#include <pthread.h>
#include <mqueue.h>
#define BUF_SIZE 100
#define MAX_CLNT 20

pthread_mutex_t mutex;
int socks[MAX_CLNT];
int clnt_count;        //record the number of connected client
void *clnt_handler(void *arg);
void send_data(char *buf,int n);

int main(int argc,char **argv)
{
	int serv_sock,clnt_sock;
	socklen_t clnt_len;
	struct sockaddr_in serv_addr,clnt_addr;
	pthread_t pth_id;

	if(argc!=2)
	{
		fprintf(stderr,"Usage:%s <port>\n",argv[0]);
		exit(1);
	}
	
	pthread_mutex_init(&mutex,NULL);
	serv_sock=socket(AF_INET,SOCK_STREAM,0);
	if(serv_sock==-1)
	{
		fprintf(stderr,"socket() error!\n");
		exit(1);
	}

	memset((void*)&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));


	if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
	{
		fprintf(stderr,"bind() error!\n");
		exit(1);
	}

	if(listen(serv_sock,5)==-1)
	{
		fprintf(stderr,"listen() error\n");
		exit(1);
	}

	clnt_count=0;
	while(1)
	{
		clnt_len=sizeof(clnt_addr);
		clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_len);

		printf("Connect client IP: %s \n",inet_ntoa(clnt_addr.sin_addr));
		pthread_mutex_lock(&mutex);
		socks[clnt_count]=clnt_sock;
		clnt_count++;
		pthread_mutex_unlock(&mutex);

		pthread_create(&pth_id,NULL,clnt_handler,&clnt_sock);
		pthread_detach(pth_id);
	}
	close(serv_sock);
	return 0;
}

void *clnt_handler(void *arg)
{
	int fd;
	int n,i;
	char buf[BUF_SIZE];

	fd=*((int*)arg);
	memset((void*)buf,0,sizeof(buf));
	while((n=read(fd,buf,BUF_SIZE))!=0)
		send_data(buf,n);

	pthread_mutex_lock(&mutex);
	for(i=0; i<clnt_count; i++)   // remove disconnected client
	{
		if(fd==socks[i])
		{
			//while(/*i++*/i<clnt_cnt-1)   //modify by myself
			//	clnt_socks[i]=clnt_socks[i+1];
			while(i<clnt_count-1)
			{
				socks[i]=socks[i+1];
				i++;
			}
			break;
		}
	}
	clnt_count--;
	pthread_mutex_unlock(&mutex);
	close(fd);
	return NULL;
}

void send_data(char *buf,int n)
{
	int i;

	pthread_mutex_lock(&mutex);
	for(i=0;i<clnt_count;i++)
	{
		write(socks[i],buf,n);
	}
	pthread_mutex_unlock(&mutex);
}
