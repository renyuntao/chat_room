#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<netdb.h>

// Set font color and style
#define NORMAL   "\x1B[0m"
#define BOLD     "\x1B[1m"   
#define BLACK    "\x1B[30m"
#define RED      "\x1B[31m"
#define GREEN    "\x1B[32m"
#define YELLOW   "\x1B[33m"
#define BLUE     "\x1B[34m"
#define MAGENTA  "\x1b[35m"
#define CYAN     "\x1b[36m"
#define LGRAY	 "\x1b[37m"
#define DGRAY	 "\x1b[90m"
#define LRED	 "\x1b[91m"
#define LGREEN	 "\x1b[92m"
#define LYELLOW  "\x1b[93m"
#define LBLUE	 "\x1b[94m"
#define LMAGENTA "\x1b[95m"
#define LCYAN	 "\x1b[96m"
#define WHITE	 "\x1b[97m"

#define COLOR_NUM 15 

char* colors[COLOR_NUM] = {RED,GREEN,YELLOW,BLUE,MAGENTA,CYAN,DGRAY,
						  LRED,LGREEN,LYELLOW,LBLUE,LMAGENTA,LCYAN,LGRAY,WHITE};

// Set maximun buffer size
#define BUFFER_SIZE 150

#define EMAIL_LEN 30
#define MAX_USERNAME_LEN 20
#define MAX_PASSWD_LEN 25
#define USER_INFO_MAX_LEN ((MAX_USERNAME_LEN)+(MAX_PASSWD_LEN))

// Define the Server domain and Port
//static const char *serverDomain = "www.studyandshare.info";
static const char *serverDomain = "localhost";
static const uint16_t serverPort = 9999;

char username[MAX_USERNAME_LEN] = {0,};
// Handle error
void printError(const char *message);

// User validation
void validation(char name[], char passwd[], int sock, char result[]);

void* send_msg(void *arg);
void* recv_msg(void *arg);
int ShowMenuGetChoice();
void SignUp(int sock);
// Client forgot password
void ForgotPasswd(int sock);

int main(void)
{
	int sock;
	struct sockaddr_in serv_addr;

	// Get Server IP
	struct hostent *host;
	host = gethostbyname(serverDomain);
	char *ServerIP = inet_ntoa(*((struct in_addr*)host->h_addr_list[0]));
	printf("ServerIP:%s\n",ServerIP);

	pthread_t send_id, recv_id;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		printError("socket() error...");

	// Populate the struct sockaddr_in `serv_addr`
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ServerIP);
	serv_addr.sin_port = htons(serverPort);

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		printError("connect() error...");

	int choice = ShowMenuGetChoice();

	switch(choice)
	{
		// Log In
		case 1:
		{
			// Send the user choice to Server
			write(sock, "1", 1);

			// User validation
			char passwd[MAX_PASSWD_LEN] = {0,};
			char result[2] = {0,};
			while(true)
			{
				validation(username, passwd, sock, result);


				// "1" denote passing user validation
				if(!strcmp(result,"1"))
				{
					system("clear");
					printf(BOLD);
					puts(CYAN"Login successfully!"NORMAL);
					break;
				}
				// "2" denote user name is not found
				else if(!strcmp(result,"2"))
				{
					system("clear");
					puts(RED"User name not exists."NORMAL);
				}
				// "3" denote password is incorrect
				else if(!strcmp(result,"3"))
				{
					system("clear");
					puts(RED"Password is incorrect."NORMAL);
				}
				// Other result is unrecognized
				else
				{
					system("clear");
					puts(RED"Unrecognized result."NORMAL);
				}
				puts(RED"Login failed..."NORMAL);

				// Make choice
				puts(DGRAY"*************** Choice ****************");
				puts("1. Try again");
				puts("2. Quit");
				printf("Make your choice: "BLUE);

				int choice;
				while(true)
				{
					scanf("%d", &choice);
					if(choice < 1 || choice > 2)
					{
						printf(RED"Invalid input, please input again: "BLUE);

						// Read the remaining character from buffer
						while(getchar() != '\n');
					}
					else
					{
						printf(NORMAL);
						break;
					}
				}

				if(choice == 1)
				{
					// Send client's choice to Server
					write(sock, "1", 1);

					// Read '\n' from buffer
					getchar();
				}
				else if(choice == 2)
				{
					// Send client's choice to Server
					write(sock, "2", 1);
					close(sock);
					return 0;
				}
			}

			// Login successfully
			puts(LMAGENTA);
			printf(BOLD);
			puts("************************* Chat Room *******************************"NORMAL);
			pthread_create(&send_id, NULL, send_msg, &sock);
			pthread_create(&recv_id, NULL, recv_msg, &sock);
			pthread_join(send_id, NULL);
			pthread_join(recv_id, NULL);
			break;
		}
		// Sign up
		case 2:
			// Send the user choice to Server
			write(sock, "2", 1);

			SignUp(sock);
			break;
		// Forgot Password
		case 3:
			// Send the user choice to Server
			write(sock, "3", 1);

			ForgotPasswd(sock);
			break;
		// Quit
		case 4:
			write(sock, "4", 1);
			close(sock);
			break;
	}

	return 0;
}

// Handle error
void printError(const char *message)
{
	printf(RED);
	fputs(message, stderr);
	fputc('\n', stderr);
	printf(NORMAL);
	exit(1);
}

// User validation
void validation(char name[], char passwd[], int sock, char result[])
{
	// Input username and password
	printf(LYELLOW"Input Username: "BLUE);
	fgets(name, MAX_USERNAME_LEN-1, stdin);
	int name_len = strlen(name);
	name[name_len-1] = '\0';
	passwd = getpass(LYELLOW"Input Password: "NORMAL);

	// Send user infomation to Server
	char userinfo[USER_INFO_MAX_LEN] = {0,};
	sprintf(userinfo, "%s,%s", name, passwd);
	write(sock, userinfo, strlen(userinfo));

	// Receive result from Server
	read(sock, result, 1);
}


// Send message from Client to Server
void* send_msg(void *arg)
{
	int fd = *((int*)arg);
	char message[BUFFER_SIZE] = {0,};
	char SendMessage[(MAX_USERNAME_LEN)+(BUFFER_SIZE)] = {0,};
	int str_len;
	while(1)
	{
		fgets(message, BUFFER_SIZE-1, stdin);
		if(!strcmp(message, "\\q\n") || !strcmp(message, "\\Q\n"))
		{
			// Half-close
			shutdown(fd, SHUT_WR);

			system("clear");
			printf(BOLD);
			printf(MAGENTA"Logout Successfully!"NORMAL);
			printf("\n");
			break;
		}

		sprintf(SendMessage, "[%s]: %s", username, message);
		write(fd, SendMessage, strlen(SendMessage));
	}
	return NULL;
}


// Receive message from Server
void* recv_msg(void *arg)
{
	int fd = *((int*)arg);
	char RecvMessage[(MAX_USERNAME_LEN)+(BUFFER_SIZE)] = {0,};
	int str_len;
	printf("                                                         Input message:\n");
	printf("                                                         ");
	fflush(stdout);

	while(1)
	{
		str_len = read(fd, RecvMessage, (MAX_USERNAME_LEN)+(BUFFER_SIZE)-1);
		if(str_len == 0)
		{
			// Half-close
			shutdown(fd, SHUT_RD);
			break;
		}
		RecvMessage[str_len-1] = '\0';

		// Extract socket id from `RecvMessage`
		char sock_num_str[3] = {0,};
		int index = 0;
		while(true)
		{
			if(RecvMessage[index] != ',')
			{
				sock_num_str[index] = RecvMessage[index];
				++index;
			}
			else
				break;
		}

		int sock_num = atoi(sock_num_str);

		// Extract message from `RecvMessage`
		strtok(RecvMessage, ",");
		char *message = strtok(NULL, ",");

		printf(BOLD);
		char *color = colors[sock_num%(COLOR_NUM)];
		printf("\n%s%s%s",color,message,NORMAL);
		memset(RecvMessage, 0, sizeof(RecvMessage));
		printf("                                                         ");
		fflush(stdout);
	}

	return NULL;
}


int ShowMenuGetChoice()
{
	system("clear");
	int choice;
	puts(DGRAY"*********************** Menu ***********************");
	puts("1. Log In");
	puts("2. Sign Up");
	puts("3. Forgot Password?");
	puts("4. Quit");
	printf("Make your choice[1-4]:"BLUE);

	while(1)
	{
		scanf("%d", &choice);
		printf(NORMAL);
		if(choice < 1 || choice > 4)
		{
			printf(RED"Invalid input, please input again:"NORMAL);
			while(getchar() != '\n');
		}
		else
			break;
	}

	getchar();
	return choice;
}


void SignUp(int sock)
{
	char email[EMAIL_LEN] = {0,};
	char username[MAX_USERNAME_LEN] = {0,};
	char *passwd;
	char *PasswdRepeat;

	printf(LYELLOW"E-mail: "BLUE);
	fgets(email, EMAIL_LEN-1, stdin);
	int str_len = strlen(email);
	email[str_len-1] = '\0';
	printf(LYELLOW"User Name: "BLUE);
	fgets(username, MAX_USERNAME_LEN-1, stdin);
	str_len = strlen(username);
	username[str_len-1] = '\0';
	passwd = getpass(LYELLOW"Password: ");
	PasswdRepeat = getpass(LYELLOW"Password(repeat): ");

	if(strcmp(passwd, PasswdRepeat))
	{
		puts(RED"Sorry, passwords do not match"NORMAL);
	}
	else
	{
		char SendMessage[(EMAIL_LEN)+(MAX_USERNAME_LEN)+(MAX_PASSWD_LEN)] = {0,};
		sprintf(SendMessage, "%s,%s,%s", username, passwd, email);
		
		// Send user information to Server
		write(sock, SendMessage, strlen(SendMessage));

		// Receive result from Server
		char result[2] = {0,};
		read(sock, result, 1);

		// "0" denote signup successfully
		if(!strcmp(result,"0"))
		{
			system("clear");
			printf(BOLD);
			puts(CYAN"Sign Up success!"NORMAL);
		}
		// "1" denote sign up failed
		else if(!strcmp(result, "1"))
		{
			puts(RED"Sign up failed."NORMAL);
		}
		else if(!strcmp(result,"2"))
		{
			puts(RED"User name is already exist, please use another name."NORMAL);
		}
		else
		{
			printf("result:%s\n",result);
			puts(RED"Unrecognized error."NORMAL);
		}
	}
	close(sock);
}

// Client forgot password
void ForgotPasswd(int sock)
{
	char username[MAX_USERNAME_LEN] = {0,};
	char email[EMAIL_LEN] = {0,};
	char SendMessage[(MAX_USERNAME_LEN)+(EMAIL_LEN)] = {0,};
	
	printf(LYELLOW"User Name: "BLUE);
	fgets(username, MAX_USERNAME_LEN-1, stdin);
	int str_len = strlen(username);
	username[str_len-1] = '\0';
	printf(LYELLOW"E-mail: "BLUE);
	fgets(email, EMAIL_LEN, stdin);

	// Sent message to Server
	sprintf(SendMessage, "%s,%s", username, email);
	write(sock, SendMessage, strlen(SendMessage));

	// Receive result from Server
	//char result[2] = {0,};
	//read(sock, result, 1);
	puts(CYAN"If the username and email you specified exists in our system, we've sent the password to your email."NORMAL);
	close(sock);

}
