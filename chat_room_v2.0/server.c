#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>

// Set maximun buffer size
#define BUFFER_SIZE 150

#define EMAIL_LEN 25
#define MAX_USERNAME_LEN 20
#define MAX_PASSWD_LEN 25
#define USER_INFO_MAX_LEN ((MAX_USERNAME_LEN)+(MAX_PASSWD_LEN))
#define MAX_CLIENT 10
#define LINE_MAX_LEN ((MAX_USERNAME_LEN)+(MAX_PASSWD_LEN)+(EMAIL_LEN))

// Define Server Port
static const uint16_t serverPort = 9999;

// Define mutex
pthread_mutex_t mutex;
pthread_mutex_t mutex_signup;

// Denote number of current client connection
int client_num = 0;

// Used in `listen()`, define the maximum length to which the queue of pending connections
const int queueMaxLen = 6;

// Store sockets of client
static int socks[MAX_CLIENT] = {0,};

// Handle error 
void printError(const char *message);

// User validation
void validation(int sock);
void* client_handler(void *arg);
void SendMessage(char message[], int len, int sock);
// Handle client sign up
void* SignUpHandler(void *arg);
// Handle client forgot password
void* ForgotPasswdHandler(void *arg);

int main(void)
{
	int serv_sock;
	int clnt_sock;
	pthread_t pthread_id;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutex_signup, NULL);

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		printError("socket() error...");
	
	// Populate the struct sockaddr_in `serv_addr`
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(serverPort);

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		printError("bind() error...");
	
	if(listen(serv_sock, queueMaxLen) == -1)
		printError("listen() error...");

	clnt_addr_size = sizeof(clnt_addr);
	while(1)
	{
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
		if(clnt_sock == -1)
		{
			puts("accept() error...");
			continue;
		}

		// Receive choice from client
		int result;
		char choice[2] = {0,};
		read(clnt_sock, choice, 1);
		if(!strcmp(choice, "1"))
			result = 1;
		else if(!strcmp(choice, "2"))
			result = 2;
		else if(!strcmp(choice, "3"))
			result = 3;
		else if(!strcmp(choice, "4"))
			result = 4;


		switch(result)
		{
			// Into chat room
			case 1:
				pthread_mutex_lock(&mutex);
				socks[client_num++] = clnt_sock;
				pthread_mutex_unlock(&mutex);

				pthread_create(&pthread_id, NULL, client_handler, &clnt_sock);
				pthread_detach(pthread_id);
				break;
			// Sign up
			case 2:
				pthread_create(&pthread_id, NULL, SignUpHandler, &clnt_sock);
				pthread_detach(pthread_id);
				break;
			// Client forgot password
			case 3:
				pthread_create(&pthread_id, NULL, ForgotPasswdHandler, &clnt_sock);
				pthread_detach(pthread_id);
				break;
			// Quit
			case 4:
				close(clnt_sock);
				break;
		}
	}
	
	close(serv_sock);
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutex_signup);
	return 0;
}

// Handle error
void printError(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

// User validation
void validation(int sock)
{
	FILE *fin;
	char userinfo[USER_INFO_MAX_LEN] = {0,};
	char *username;
	char *passwd;
	bool pass = false;
	bool NotFoundUserName = true;

	while(true)
	{
		// Receive user infomation from Client
		read(sock, userinfo, USER_INFO_MAX_LEN-1);


		// Store username and password
		username = strtok(userinfo, ",");
		passwd = strtok(NULL, ",");

		fin = fopen("userinfo.txt", "r");
		if(fin == NULL)
			printError("fopen() error...");

		// Read from file `userinfo.txt` line by line
		char line[LINE_MAX_LEN] = {0,};
		char *tmp;
		while(fgets(line, LINE_MAX_LEN-1, fin))
		{
			tmp = strtok(line, ",");

			// Match username
			if(!strcmp(tmp, username))
			{
				NotFoundUserName = false;
				tmp = strtok(NULL, ",");

				// Match password
				if(!strcmp(tmp, passwd))
				{
					pass = true;
					break;
				}
				else
				{
					break;
				}
			}
		}

		// Passing user validation
		if(pass)	
		{
			// "1" denote passing user validation
			write(sock, "1", 1);
			break;
		}
		// User name not found
		else if(NotFoundUserName)
		{
			// "2" denote not found user name
			write(sock, "2", 1);
		}
		// User password is not match
		else
		{
			// "3" denote password is incorrect
			write(sock, "3", 1);
		}
		
		char choice[2] = {0,};
		read(sock, choice, 1);

		// Client try again
		if(!strcmp(choice, "1"))
		{
			continue;
		}
		// Client quit
		else if(!strcmp(choice, "2"))
		{
			break;
		}
	}

	fclose(fin);
}

// Handle client connection
void* client_handler(void *arg)
{
	int fd = *((int*)arg);

	// User validation
	validation(fd);

	int str_len;
	char message[BUFFER_SIZE] = {0,};
	while((str_len = read(fd, message, BUFFER_SIZE-1)) != 0)
		SendMessage(message, str_len, fd);

	// Run when disconnect
	pthread_mutex_lock(&mutex);
	for(int i = 0; i < client_num; ++i)
	{
		if(socks[i] == fd)
		{
			while(i < client_num-1)
			{
				socks[i] = socks[i+1];
				++i;
			}
			break;
		}
	}
	--client_num;
	pthread_mutex_unlock(&mutex);
	close(fd);
	return NULL;
}

// Send message to every client
void SendMessage(char message[], int len, int sock)
{
	pthread_mutex_lock(&mutex);
	for(int i = 0; i < client_num; ++i)
	{
		// Prefix sock to `message`
		char SendMessage[(BUFFER_SIZE)+3] = {0,};
		sprintf(SendMessage,"%d,%s",sock,message);

		write(socks[i], SendMessage, len+3);
	}
	pthread_mutex_unlock(&mutex);
}

// Handle client sign up
void* SignUpHandler(void *arg)
{
	int fd = *((int*)arg);
	char userinfo[USER_INFO_MAX_LEN] = {0,};
	char *filename = "userinfo.txt";
	char *username = NULL;

	// Read client sign up information from client
	read(fd, userinfo, USER_INFO_MAX_LEN);

	char userinfo_cp[MAX_USERNAME_LEN] = {0,};
	strcpy(userinfo_cp, userinfo);

	// Extract username from `userinfo`
	username = strtok(userinfo_cp, ",");

	pthread_mutex_lock(&mutex_signup);
	FILE *fpout;
	fpout = fopen(filename, "a+");
	if(fpout == NULL)
	{
		// "1" denote sign up failed
		write(fd, "1", 1);
		close(fd);
		return NULL;
	}

	// Check whether the username is already exist
	char line[USER_INFO_MAX_LEN] = {0,};
	char *name;
	while(fgets(line, USER_INFO_MAX_LEN-1, fpout) != NULL)
	{
		name = strtok(line, ",");
		if(!strcmp(username, name))
		{
			// "2" denote the username is already exist
			write(fd,"2",1);
			fclose(fpout);
			pthread_mutex_unlock(&mutex_signup);
			return NULL;
		}
	}

	fputs(userinfo, fpout);
	fputc('\n', fpout);
	fclose(fpout);
	pthread_mutex_unlock(&mutex_signup);

	// "0" denote sign up successfully
	write(fd, "0", 1);
	close(fd);
	return NULL;
}

// Handle client forgot password
void* ForgotPasswdHandler(void *arg)
{
	int fd = *((int*)arg);
	char RecvMessage[(MAX_USERNAME_LEN)+(EMAIL_LEN)] = {0,};

	read(fd, RecvMessage, (MAX_USERNAME_LEN)+(EMAIL_LEN)-1);
	// Extract username and email from `RecvMessage`
	char *username = strtok(RecvMessage, ",");
	char *email = strtok(NULL, ",");

	char line[LINE_MAX_LEN] = {0,};
	char line_cp[LINE_MAX_LEN] = {0,};
	char *passwd = NULL;
	char *column = NULL;
	char *filename = "userinfo.txt";

	FILE *fpin;
	fpin = fopen(filename, "r");
	while(fgets(line, LINE_MAX_LEN, fpin))
	{
		strcpy(line_cp, line);
		column = strtok(line_cp, ",");
		// Match username
		if(!strcmp(username, column))
		{
			strtok(line, ",");
			passwd = strtok(NULL, ",");
			column = strtok(NULL, ",");
			
			// Match email
			if(!strcmp(email, column))
			{
				// Construct command
				char command[80] = {0,};
				strcpy(command, "./SendMail.sh");
				strcat(command, " ");
				int str_len = strlen(email);
				email[str_len-1] = '\0';
				strcat(command, email);
				strcat(command, " MailMessage.txt");

				FILE* fpout = fopen("MailMessage.txt","w");
				fputs("Your password:", fpout);
				fputs(passwd, fpout);
				fclose(fpout);

				system(command);
			}
			else
				puts("Not match email");
			break;
		}
	}
	fclose(fpin);
	close(fd);
}
