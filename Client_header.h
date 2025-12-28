#include <stdio.h>      // Standard I/O library for input and output functions
#include <sys/stat.h>   // Header for file status and mode constants
#include <sys/wait.h>   // Header for process waiting functions
#include <fcntl.h>      // Header for file control options
#include <string.h>     // Header for string manipulation functions
#include <unistd.h>     // Header for POSIX operating system API
#include <stdlib.h>     // Header for general utilities like memory allocation
#include <sys/types.h>  // Header for data types used in system calls
#include <sys/socket.h> // Header for socket programming
#include <arpa/inet.h>  // Header for internet operations (e.g., IP addresses)
#include <ctype.h>      // Header for character handling functions
#include <pthread.h>    // Header for POSIX threads
#include <stdio_ext.h>  // Header for extended I/O functions
#include <termios.h>
#include <unistd.h>
#include <time.h>
#define MAX_USERS 10 // Maximum number of users allowed in the chat room

pthread_t recv_thread;
pthread_t send_thread;

int recv_running = 1;
int recv_thread_started = 0;
int group_chat_running = 0;

typedef struct
{
    int sockfd;                     // Socket file descriptor
    struct sockaddr_in server_addr; // Server address structure
    unsigned int server_addr_len;   // Length of the server address structure
} Client_struct;

enum
{
    LOGIN = 1,
    REGISTER,
    LOGOUT,
    ONLINE_USERS_LIST,
    PRIVATE_CHAT,
    GROUP_CHAT,
    OFFLINE_USERS_LIST,
    ERROR,
    PRIVATE_CHAT_EXIT,
    GROUP_CHAT_EXIT
} Type;

typedef struct
{
    int type;
    struct
    {
        char username[100];
        char password[100];
        char from_username[100];
    } user;

    struct
    {
        char message[1000];
    } chat_packet;

    struct
    {
        char error_message[100];
    } error_packet;

    struct
    {
        int user_count;
        int sock_fd[MAX_USERS];
        char username[MAX_USERS][20];
    } online_users_packet;
} chatroom_packet;

typedef struct
{
    int sockfd;
    chatroom_packet packet; // contains username
} group_thread_arg_t;

typedef struct
{
    int sockfd;
    char peer_name[20];
    chatroom_packet packet; // contains username
} private_thread_arg_t;

void login_menu(Client_struct *client, chatroom_packet *packet); // Function to display the login menu
void Register(Client_struct *client, chatroom_packet *packet);   // Function to handle user registration
void Login(Client_struct *client, chatroom_packet *packet);      // Function to handle user login
void Read_inputs(chatroom_packet *packet);                       // Function to read user inputs
int print_online_users(Client_struct *client, chatroom_packet *packet);
void chat_menu(Client_struct *client, chatroom_packet *packet); // Function to display the chat menu
void group_chat_menu(int sockfd, chatroom_packet *login_packet);
void *group_send_thread(void *arg);
void *group_receive_thread(void *arg);
void *private_send_thread(void *arg);
void single_user_chat_menu(int sockfd, chatroom_packet *login_packet);