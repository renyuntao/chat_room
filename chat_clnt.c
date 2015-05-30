#include"my_header.h"
#define BUF_SIZE 100
#define NAME_SIZE 10

char name[NAME_SIZE];
void *send_msg(void *arg);
void *recv_msg(void *arg);

int main(int argc,char **argv)
{
	int clnt_sock;
	struct sockaddr_in serv_addr;
	pthread_t send_id,recv_id;

	if(argc!=4)
	{
		fprintf(stderr,"Usage:%s <IP> <port> <name>\n",argv[0]);
		exit(1);
	}
	
	clnt_sock=socket(AF_INET,SOCK_STREAM,0);
	if(clnt_sock==-1)
	{
		fprintf(stderr,"socket() error!\n");
		exit(1);
	}

	memset((void*)&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	//serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	inet_pton(AF_INET,argv[1],&serv_addr.sin_addr.s_addr);
	serv_addr.sin_port=htons(atoi(argv[2]));
	
	memset((void*)name,0,sizeof(name));
	sprintf(name,"[%s]",argv[3]);

	if(connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
	{
		fprintf(stderr,"connect error!\n");
		exit(1);
	}

	pthread_create(&send_id,NULL,send_msg,(void*)&clnt_sock);
	pthread_create(&recv_id,NULL,recv_msg,(void*)&clnt_sock);
	pthread_join(send_id,NULL);
	pthread_join(recv_id,NULL);
	close(clnt_sock);
	return 0;
}

void *send_msg(void *arg)
{
	int fd;
	char msg[BUF_SIZE+NAME_SIZE];
	char buf[BUF_SIZE];

	memset((void*)msg,0,sizeof(msg));
	memset((void*)buf,0,sizeof(buf));
	fd=*((int*)arg);
	while(1)
	{
	    fgets(buf,BUF_SIZE,stdin);
		if(strcmp(buf,"Q\n")==0 || strcmp(buf,"q\n")==0)
		{
			printf("exit chat root!\n");
			close(fd);
			exit(0);
		}
		sprintf(msg,"%s %s",name,buf);
		write(fd,msg,strlen(msg));
	}
	return NULL;
}

void *recv_msg(void *arg)
{
	int fd;
	int str_len;
	char buf[BUF_SIZE+NAME_SIZE];

	fd=*((int*)arg);
	memset((void*)buf,0,sizeof(buf));
	while(1)
	{
		str_len=read(fd,buf,BUF_SIZE+NAME_SIZE);
		if(str_len==-1)
		{
			fprintf(stderr,"read() error!\n");
			exit(1);
		}
		buf[str_len]=0;
		fputs(buf,stdout);
	}
	return NULL;
}
