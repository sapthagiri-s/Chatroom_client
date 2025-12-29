#include "Client_header.h"

Client_struct client; // Declare a variable of type Client_struct
chatroom_packet packet;
int choice2 = 0, flag = 0;
char before_username[100];
void handle_sigint(int sig)
{
	printf("\n[INFO] Disconnecting from server...\n");
	packet.type = LOGOUT; // Set the packet type to LOGOUT
	send(client.sockfd, &packet, sizeof(packet), 0);
	sleep(1); // Or use a proper ack-based exit if you have time
	close(client.sockfd);
	printf("[INFO] Client disconnected. Bye 👋\n");
	exit(0);
}

void print_time()
{
	time_t now;
	struct tm *tm_info;

	time(&now);
	tm_info = localtime(&now);

	printf("[%02d:%02d] ", tm_info->tm_hour, tm_info->tm_min);
}

int main()
{
	signal(SIGINT, handle_sigint);					 // Declare a variable of type chatroom_packet
	char buffer[1024];								 // Buffer to store received data
	client.sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket for TCP communication
	if (client.sockfd < 0)							 // Check if socket creation failed
	{
		perror("Socket creation failed"); // Print error message
		exit(EXIT_FAILURE);				  // Exit the program with failure status
	}
	client.server_addr.sin_family = AF_INET;															// Set address family to IPv4
	client.server_addr.sin_port = htons(6007);															// Set port number (converted to network byte order)
	client.server_addr.sin_addr.s_addr = inet_addr("172.30.244.176");									// Set server IP address
	if (connect(client.sockfd, (struct sockaddr *)&client.server_addr, sizeof(client.server_addr)) < 0) // Attempt to connect to the server
	{
		perror("Connection failed"); // Print error message if connection fails
		close(client.sockfd);		 // Close the socket
		exit(EXIT_FAILURE);			 // Exit the program with failure status
	}
	printf("\n==================================================\n\t\tTCP CHAT ROOM CLIENT\n==================================================\nServer   : %s\nPort	 : %d\nStatus   : Connected ✅\n==================================================\n", inet_ntoa(client.server_addr.sin_addr), ntohs(client.server_addr.sin_port));
	login_menu(&client, &packet); // Call the login menu function
	close(client.sockfd);		  // Close the socket
}

void login_menu(Client_struct *client, chatroom_packet *packet)
{
	int choice1 = 0;
	while (1)
	{
		printf("\n1️⃣  Login\n2️⃣  Register\n3️⃣  Exit\n\n--------------------------------------------------\n 👉 Enter your option : ");
		__fpurge(stdin);
		scanf("%d", &choice1); // Read user choice
		switch (choice1)	   // Switch case to handle user choice
		{
		case 1:
			Login(client, packet);												   // Call the Login
			recv(client->sockfd, packet, sizeof(*packet), 0);					   // Receive the packet from the server
			if (!strncmp(packet->error_packet.error_message, "login_success", 13)) // Check if login was successful
			{
				printf(" ✅ Login successful. Welcome %s!\n\n", packet->user.username);
				if (!recv_thread_started)
				{
					recv_running = 1;
					pthread_create(&recv_thread, NULL, group_receive_thread, client);
					recv_thread_started = 1;
				}
				chat_menu(client, packet); // Call the chat menu function
			}
			else if (!strncmp(packet->error_packet.error_message, "login_failed", 13)) // Check if login failed
			{
				printf(" ❌ Invalid username or password\n"); // Print login failure message
			}
			else if (!strncmp(packet->error_packet.error_message, "Username does not exist", 23)) // Check if username does not exist
			{
				printf(" ❌ Username does not exist. Please register first.\n"); // Print username does not exist message
			}
			break;
		case 2:
			Register(client, packet);												  // Call the Register function
			recv(client->sockfd, packet, sizeof(*packet), 0);						  // Receive the packet from the server
			if (!strncmp(packet->error_packet.error_message, "register_success", 16)) // Check if registration was successful
			{
				printf("\nINFO: User Registered Successful\n\n"); // Print registration success message
				packet->type = LOGIN;							  // Set the packet type to LOGIN
				send(client->sockfd, packet, sizeof(*packet), 0); // Send the packet to the server
				recv(client->sockfd, packet, sizeof(*packet), 0); // Receive the packet from the server
				chat_menu(client, packet);						  // Call the chat menu function
			}
			else if (!strncmp(packet->error_packet.error_message, "register_failed", 16)) // Check if registration failed
			{
				printf("\n[FAIL] Registration failed\n\n"); // Print registration failure message
			}
			else if (!strncmp(packet->error_packet.error_message, "Username already exists", 23)) // Check if username already exists
			{
				printf("\n[INFO] User already exists\n\n");
			}
			break;
		case 3:
			handle_sigint(SIGINT); // Call the handle_sigint function to exit
			break;
		default:
			printf("Invalid choice. Please try again.\n"); // Print invalid choice message
			break;
		}
	}
}

void Login(Client_struct *client, chatroom_packet *packet)
{
	packet->type = LOGIN; // Set the packet type to LOGIN
	printf("==================================================\n\t\tLOGIN\n==================================================\n");
	Read_inputs(packet);
	send(client->sockfd, packet, sizeof(*packet), 0); // Send the packet to the server
}

void get_password(chatroom_packet *packet)
{
	struct termios old, new;
	int i = 0;
	char ch;

	tcgetattr(STDIN_FILENO, &old); // save old settings
	new = old;
	new.c_lflag &= ~(ECHO); // disable echo
	tcsetattr(STDIN_FILENO, TCSANOW, &new);

	while ((ch = getchar()) != '\n')
	{
		packet->user.password[i++] = ch;
		printf("*"); // show *
	}
	packet->user.password[i] = '\0';

	tcsetattr(STDIN_FILENO, TCSANOW, &old); // restore
	printf("\n");
}

void Read_inputs(chatroom_packet *packet)
{
	if (packet->type == LOGIN)
	{
		printf("\nUsername : ");			// Prompt for username
		scanf("%s", packet->user.username); // Read username from input
		printf("Password : ");				// Prompt for password
		getchar();							// Consume the newline character left by previous scanf
		get_password(packet);				// Read password without echoing
		printf("--------------------------------------------------\n [✔] Authenticating...\n\n");
	}
	else if (packet->type == REGISTER)
	{
		printf("\nNew Username : ");		// Prompt for username
		scanf("%s", packet->user.username); // Read username from input
		printf("New Password : ");			// Prompt for password
		getchar();							// Consume the newline character left by previous scanf
		get_password(packet);				// Read password without echoing
		printf("--------------------------------------------------\n [✔] Creating account...\n\n");
	}
}

void Register(Client_struct *client, chatroom_packet *packet)
{
	packet->type = REGISTER; // Set the packet type to REGISTER
	printf("==================================================\n\t\tREGISTER\n==================================================\n");
	Read_inputs(packet);							  // Call the function to read user inputs
	send(client->sockfd, packet, sizeof(*packet), 0); // Send the packet to the server
}

void chat_menu(Client_struct *client, chatroom_packet *packet)
{
	printf("=================== CHAT MENU ====================\n\n 1️⃣  Single User Chat\n 2️⃣  Multi User Chat\n 3️⃣  Logout\n\n==================================================\n👉 Select chat mode : ");
	__fpurge(stdin);
	scanf("%d", &choice2); // Read user choice
	switch (choice2)	   // Switch case to handle user choice
	{
	case 1:
		printf("==================================================\n\t\tCHAT ROOM\n==================================================\n Logged in User : %s\n", packet->user.username);
		single_user_chat_menu(client->sockfd, packet); // Call the single user chat menu function
		chat_menu(client, packet);					   // Call the chat menu function again
		break;
	case 2:
		printf("==================================================\n\t\tCHAT ROOM\n==================================================\n Logged in User : %s\n", packet->user.username);
		group_chat_menu(client->sockfd, packet); // Call the group chat menu function
		chat_menu(client, packet);				 // Call the chat menu function again
		break;
	case 3:
		sleep(1);							  // Or use a proper ack-based exit if you have time
		printf("Exiting.. from chat menu\n"); // Print exit message
		login_menu(client, packet);			  // Call the login menu function
		break;
	default:
		printf("Invalid choice. Please try again.\n"); // Print invalid choice message
		choice2 = 0;
		chat_menu(client, packet);
		break;
	}
}

int print_online_users(Client_struct *client, chatroom_packet *packet)
{
	printf(" Online Users  : %d\n", packet->online_users_packet.user_count);
	if (packet->online_users_packet.user_count == 0)
	{
		printf("==================================================\nType message ( /exit to quit or /Users to list online users):\n");
		return 0;
	}
	else if (packet->type == ONLINE_USERS_LIST) // Check if the packet type is ONLINE_USERS_LIST
	{
		for (int i = 0; i < packet->online_users_packet.user_count; i++) // Loop through the online users
		{
			printf("  - %s\n", packet->online_users_packet.username[i]); // Print online user
		}
		printf("==================================================\nType message ( /exit to quit or /Users to list online users):\n");
	}
}
void single_user_chat_menu(int sockfd, chatroom_packet *login_packet)
{
	login_packet->type = ONLINE_USERS_LIST;
	send(sockfd, login_packet, sizeof(*login_packet), 0);
	sleep(1);
	private_thread_arg_t *send_ctx1 = malloc(sizeof(*send_ctx1));
	if (!send_ctx1)
		return;
	if (flag == 0)
	{
		printf("\nEnter the username of the person you want to chat with: ");
		scanf("%s", send_ctx1->peer_name);
		send_ctx1->sockfd = sockfd;
		memcpy(&send_ctx1->packet, login_packet, sizeof(chatroom_packet));
		strcpy(send_ctx1->packet.user.from_username, login_packet->user.username);
	}
	else
	{
		send_ctx1->sockfd = sockfd;
		memcpy(&send_ctx1->packet, login_packet, sizeof(chatroom_packet));
		strcpy(send_ctx1->peer_name, before_username);
		strcpy(send_ctx1->packet.user.from_username, login_packet->user.username);
		flag = 0;
	}

	group_chat_running = 1;

	pthread_create(&send_thread, NULL, private_send_thread, send_ctx1);
	pthread_join(send_thread, NULL);
	group_chat_running = 0;
}
void *group_receive_thread(void *arg)
{
	Client_struct *client = (Client_struct *)arg;
	chatroom_packet packet;

	while (recv_running)
	{
		if (recv(client->sockfd, &packet, sizeof(packet), 0) <= 0)
			break;

		if (packet.type == GROUP_CHAT)
		{
			choice2 = 2;
			printf("\n");
			print_time();
			printf("%s: %s\n> ", packet.user.username, packet.chat_packet.message);
			fflush(stdout);
		}
		else if (packet.type == ONLINE_USERS_LIST)
		{
			print_online_users(client, &packet);
			fflush(stdout);
		}
		else if (packet.type == PRIVATE_CHAT)
		{
			flag = 1;
			choice2 = 1;
			printf("\n");
			print_time();
			printf("%s: %s\n> ", packet.user.from_username, packet.chat_packet.message);
			strcpy(before_username, packet.user.from_username);
			fflush(stdout);
		}
		else if (packet.type == LOGOUT)
		{
			printf("\n[INFO] Logged out by server. Disconnecting...\n");
			recv_running = 0;
			close(client->sockfd);
			printf("[INFO] Client disconnected. Bye 👋\n");
			exit(0);
		}
	}

	pthread_exit(NULL);
}

void group_chat_menu(int sockfd, chatroom_packet *login_packet)
{
	login_packet->type = ONLINE_USERS_LIST;
	send(sockfd, login_packet, sizeof(*login_packet), 0);
	sleep(1);
	/* ---- Sender thread context (packet + socket) ---- */
	group_thread_arg_t *send_ctx = malloc(sizeof(*send_ctx));
	if (!send_ctx)
		return;

	send_ctx->sockfd = sockfd;
	memcpy(&send_ctx->packet, login_packet, sizeof(chatroom_packet));

	/* ---- Receiver thread argument (only socket) ---- */
	int *recv_sock = malloc(sizeof(int));
	if (!recv_sock)
	{
		free(send_ctx);
		return;
	}
	*recv_sock = sockfd;

	group_chat_running = 1;

	pthread_create(&send_thread, NULL, group_send_thread, send_ctx);
	pthread_join(send_thread, NULL);
	group_chat_running = 0;
}

void *group_send_thread(void *arg)
{
	group_thread_arg_t *ctx = (group_thread_arg_t *)arg;
	chatroom_packet *packet = &ctx->packet;

	packet->type = GROUP_CHAT;
	while (group_chat_running)
	{
		if (packet->type == GROUP_CHAT)
			printf("> ");
		fflush(stdout);
		scanf(" %[^\n]", packet->chat_packet.message);
		if (!strcmp(packet->chat_packet.message, "/exit"))
		{
			printf("Exiting group chat...\n");
			group_chat_running = 0;
			shutdown(ctx->sockfd, SHUT_RD); // To unblock the
			pthread_join(recv_thread, NULL);
			packet->type = GROUP_CHAT_EXIT;
			send(ctx->sockfd, packet, sizeof(*packet), 0);
			break;
		}
		else if (!strcmp(packet->chat_packet.message, "/Users"))
		{
			printf("\n==================================================\n");
			packet->type = ONLINE_USERS_LIST;
			send(ctx->sockfd, packet, sizeof(*packet), 0);
			continue;
		}
		packet->type = GROUP_CHAT;
		send(ctx->sockfd, packet, sizeof(*packet), 0);
	}
	free(ctx);
	pthread_exit(NULL);
}

void *private_send_thread(void *arg)
{
	private_thread_arg_t *ctx = (private_thread_arg_t *)arg;
	chatroom_packet packet;

	packet.type = PRIVATE_CHAT;
	strcpy(packet.user.username, ctx->peer_name);
	strcpy(packet.user.from_username, ctx->packet.user.from_username);

	while (1)
	{
		if (packet.type == PRIVATE_CHAT && flag == 0)
			printf("> ");
		scanf(" %[^\n]", packet.chat_packet.message);

		if (!strcmp(packet.chat_packet.message, "/exit"))
		{
			printf("Exiting private chat...\n");
			packet.type = PRIVATE_CHAT_EXIT;
			send(ctx->sockfd, &packet, sizeof(packet), 0);
			break;
		}
		else if (!strcmp(packet.chat_packet.message, "/Users"))
		{
			printf("\n==================================================\n");
			packet.type = ONLINE_USERS_LIST;
			send(ctx->sockfd, &packet, sizeof(packet), 0);
			continue;
		}
		packet.type = PRIVATE_CHAT;
		send(ctx->sockfd, &packet, sizeof(packet), 0);
	}

	free(ctx);
	pthread_exit(NULL);
}
