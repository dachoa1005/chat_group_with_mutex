#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"

void send_msg_to_serv(int socket, char *client_input);
void send_file(int socket, char *file_path);

void *send_message(void *client_sockfd)
{
    int socket = *(int *)client_sockfd;
    char client_name[1024];

    // enter name and check then send to server
    while (1)
    {
        printf("Enter your name: ");
        fgets(client_name, 1024, stdin);
        client_name[strlen(client_name) - 1] = '\0';
        if (strcmp(client_name, "") != 0)
            break;
    }
    if (send(socket, client_name, strlen(client_name), 0) < 0)
    {
        perror("send");
        exit(1);
    }

    char client_input[1024];
    char file_path[1024];
    while (1)
    {
        memset(client_input, 0, 1024);
        // memset(file_path, 0, sizeof(file_path));
        
        fgets(client_input, 1024, stdin);
        client_input[strlen(client_input) - 1] = '\0';
        if (strcmp(client_input, "")== 0)
            continue;

        if (strncmp(client_input, "/upload ", strlen("/upload ")) == 0)
        {
            sscanf(client_input, "%*s %s", file_path);
            printf("file_path: %s\n", file_path);
            if (strcmp(file_path, "") == 0)
            {
                printf("Usage: /upload <file-path>");
                continue;
            } else 
            {
                send_file(socket, file_path); //check if fopen ==NULL
            }
            memset(file_path, 0, sizeof(file_path));
        } else // client_input = message
        {
            printf("client_input: %s\n", client_input);
            send_msg_to_serv(socket, client_input); 
        }
    }
}

void *recv_message(void *client_sockfd)
{
    int socket = *(int *)client_sockfd;
    char message[1024];
    char msg[1024];
    while (1)
    {
        int recv_len = recv(socket, message, 1024, 0);
        if (recv_len < 0)
        {
            perror("recv");
            exit(1);
        }
        if (recv_len == 0)
        {
            printf("Server disconnected\n");
            exit(1);
        }
        message[recv_len] = '\0';

        // check if message is a txt or file? (format: TXT|message or FILE|file_size|data)
        printf("%s\n", message);
        if (strncmp(message, "TXT|", strlen("TXT|")) == 0) //this is a Text
        {
            sscanf(message, "%*s|%s", msg);
            printf("%s\n", msg);
            continue;
        }
    }
}

void send_file(int socket, char *file_path)
{
    return;
}

void send_msg_to_serv(int socket, char *client_input)
{
    // char *msg = malloc(sizeof("TXT|")+ sizeof(client_input));
    char msg[1024];
    sprintf(msg, "%s%s", "TXT|", client_input);
    if (send(socket, msg, strlen(msg), 0) < 0)
    {
        perror("send");
        exit(1);
    }
    return;
}

int main(int argc, char *argv[])
{
    pthread_t send_thread, recv_thread;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(port);

    if (connect(client_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    printf("Connected to server\n");

    pthread_create(&send_thread, NULL, send_message, (void *)&client_sockfd);
    pthread_create(&recv_thread, NULL, recv_message, (void *)&client_sockfd);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    return 0;
}