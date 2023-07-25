#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_CLIENTS 1000
#define BUFFER_SIZE 1024

int counter;
pthread_mutex_t lock;
pthread_mutex_t client_number_lock;

typedef struct
{
    int sockfd;
    char *name;
} Client;

Client clients[MAX_CLIENTS];
int client_number = 0;

void send_to_all(int socket, char *buffer)
{
    pthread_mutex_lock(&lock); // Protect clients array while sending
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sockfd != socket && clients[i].sockfd > 0 && clients[i].name != NULL)
        {
            send(clients[i].sockfd, buffer, strlen(buffer), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void *connection_handle(void *arg)
{
    int socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char *client_name;
    int read_len = 0;
    char msg[BUFFER_SIZE];
    char temp[BUFFER_SIZE];
    char print_msg[BUFFER_SIZE];
    char file_name[800];
    char file_path[BUFFER_SIZE];
    int file_size;

    // Get client name
    read_len = recv(socket, buffer, BUFFER_SIZE, 0);
    if (read_len <= 0)
    {
        // Client disconnected before sending the name
        close(socket);
        pthread_mutex_lock(&client_number_lock);
        for (int j = 0; j < MAX_CLIENTS; j++)
        {
            if (socket == clients[j].sockfd)
            {
                clients[j].sockfd = -1;
                break;
            }
        }
        pthread_mutex_unlock(&client_number_lock);
        return NULL;
    }

    buffer[read_len] = '\0';
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
        if (read_len <= 0)
        {
            // Client disconnected
            break;
        }
        if (strcmp(buffer, "") == 0)
            continue;
        buffer[read_len] = '\0';
        // printf("buffer: %s\n", buffer);

        if (read_len > 0)
        {
            memset(print_msg, 0, BUFFER_SIZE);
            memset(temp, 0, BUFFER_SIZE);
            pthread_mutex_lock(&lock);
            counter += 1;
            pthread_mutex_unlock(&lock);

            strcat(print_msg, client_name);
            strcat(print_msg, ": ");

            if (strncmp(buffer, "TXT|", strlen("TXT|")) == 0)
            {
                int length = strlen("TXT|");
                memcpy(temp, buffer + length, strlen(buffer) - length + 1);
                strcat(print_msg, temp);
                printf("%d. %s\n\n", counter, print_msg);
                send_to_all(socket, print_msg);
                continue;
            }
            else if (strncmp(buffer, "FILE|", strlen("FILE|")) == 0)
            {
                sscanf(buffer, "%*[^|]|%d|%s|", &file_size, file_name);
                printf("%d. file_name:%s, file_size: %d\n", counter, file_name, file_size);
                int total_bytes_recv = 0;
                int bytes_recv = 0;
                int file_descriptor;
                sprintf(file_path, "./Server/%s", file_name);
                printf("file path: %s\n", file_path);

                file_descriptor = open(file_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

                if (file_descriptor == -1)
                {
                    perror("Error opening file");
                    continue;
                }
                while (total_bytes_recv < file_size)
                {
                    memset(buffer, 0, BUFFER_SIZE);
                    bytes_recv = recv(socket, buffer, BUFFER_SIZE, 0);
                    if (bytes_recv == -1)
                    {
                        printf("Error receiving file\n");
                        continue;
                    }
                    ssize_t bytes_written = write(file_descriptor, buffer, bytes_recv);

                    if (bytes_written == -1)
                    {
                        perror("Error writing to file");
                        close(file_descriptor);
                        continue;
                    }
                    total_bytes_recv += bytes_recv;
                    // printf("%s", buffer);
                }
                // printf("\ntotal bytes recv: %d\n", total_bytes_recv);
                close(file_descriptor);
                // printf("\n\n");
                continue;
            }
        }
    } while (read_len > 0);

    // Client disconnected
    printf("Client %s has closed the connection\n", client_name);

    // Delete client from clients array
    pthread_mutex_lock(&client_number_lock);
    for (int j = 0; j < MAX_CLIENTS; j++)
    {
        if (socket == clients[j].sockfd)
        {
            clients[j].sockfd = -1;
            free(clients[j].name);
            clients[j].name = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&client_number_lock);

    close(socket);
    return NULL;
}

int main(int argc, char const *argv[])
{
    // Init clients sockfd array
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

    // Init mutex locks
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&client_number_lock, NULL) != 0)
    {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
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

        // Lock the client_number mutex to safely update client number and create a thread
        pthread_mutex_lock(&client_number_lock);
        clients[client_number].sockfd = client_sockfd;
        printf("Client has socketfd: %d has connected.\n\n", client_sockfd);

        // Create thread to handle each client
        if (pthread_create(&threads[client_number], NULL, connection_handle, (void *)&client_sockfd) != 0)
        {
            perror("pthread_create");
            pthread_mutex_unlock(&client_number_lock);
            exit(EXIT_FAILURE);
        }

        client_number += 1;
        pthread_mutex_unlock(&client_number_lock);
    }

    // Destroy the mutexes
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&client_number_lock);

    return 0;
}
