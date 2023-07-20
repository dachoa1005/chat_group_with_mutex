#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 1000
#define BUFFER_SIZE 1024

int counter;
pthread_mutex_t lock;

typedef struct
{
    int sockfd;
    char *name;
} Client;

Client clients[MAX_CLIENTS];
int client_number = 0;

void *connection_handle(void *client_sockfd)
{
    int socket = *(int *)client_sockfd; // get client_sockfd value
    char buffer[BUFFER_SIZE];
    char *client_name;
    int read_len = 0;
    char msg[BUFFER_SIZE];
    char temp[BUFFER_SIZE];

    // get client name
    recv(socket, buffer, BUFFER_SIZE, 0);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sockfd == socket)
        {
            clients[i].name = malloc(strlen(buffer) + 1);
            strcpy(clients[i].name, buffer);
            client_name = malloc(strlen(buffer) + 1);
            strcpy(client_name, buffer);
            break;
        }
    }

    printf("Client %s has joined the chat\n", client_name);

    do
    {
        memset(buffer, 0, BUFFER_SIZE);
        read_len = recv(socket, buffer, BUFFER_SIZE, 0);
        // end of string marker
        buffer[read_len] = '\0';
        // printf("buffer: %s\n", buffer);

        if (read_len > 0)
        {
            memset(temp, 0, BUFFER_SIZE);
            // add client name to buffer
            pthread_mutex_lock(&lock);
            counter += 1;
            pthread_mutex_unlock(&lock);
            // sprintf(temp, "%d. ", counter);
            strcat(temp, client_name);
            strcat(temp, ": ");

            // handle buffer
            if (strncmp(buffer, "TXT|", strlen("TXT|")) == 0)
            {
                sscanf(buffer, "%*[^|]|%s", )
            } 
            else if (strncmp(buffer, "FILE|", strlen("FILE|")) == 0)
            {
                
            }

        }
        else
        {
            printf("Client %s has closed the connection\n", client_name);
            // delete client from clients array
            for (int j = 0; j < MAX_CLIENTS; j++)
            {
                if (socket == clients[j].sockfd)
                {
                    clients[j].sockfd = -1;
                    break;
                }
            }
        }
    } while (read_len > 0);

    return 0;
}

int main(int argc, char const *argv[])
{
    // init clients sockfd array
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].sockfd = -1;
        clients[i].name = NULL;
    }

    if (argc < 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_address;
    int addrlen = sizeof(server_address);
    char buffer[BUFFER_SIZE] = {0};
    pthread_t threads[MAX_CLIENTS];
    int thread_count = 0;

    // Create server socket
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("Server socket created.\n");

    // Set socket option
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    // Bind socket to port
    if (bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket bind to port %d.\n", port);

    // Listen for incoming connections
    if (listen(server_sockfd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // init mutex lock
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    while (1)
    {
        printf("Listening on port %d\n", port);
        // Accept connection from client
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&server_address, (socklen_t *)&addrlen);
        if (client_sockfd < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        clients[client_number].sockfd = client_sockfd;
        client_number += 1;
        printf("Client has socketfd: %d has connected.\n\n", client_sockfd);

        // create thread to handle each client
        if (pthread_create(&threads[client_number], NULL, connection_handle, (void *)&client_sockfd) != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}