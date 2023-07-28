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

#define MAX_CLIENTS 10000
#define BUFFER_SIZE 1024

int counter;
pthread_mutex_t counter_lock;

pthread_mutex_t client_number_lock;

typedef struct
{
    int sockfd;
    char *name;
} Client;

Client *clients;
int client_number = 0;
int current_client_number = 0;

void send_file(int socket, char *file_name, char *file_path);
void recv_file(int socket, int file_size, char *file_path);
char handle_client_disconnection(int socket, char *client_name);
void send_to_all(int socket, char *buffer);

void *connection_handle(void *arg)
{
    int socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char *client_name;
    int read_len = 0;
    char msg[BUFFER_SIZE];
    char temp[BUFFER_SIZE];
    char msg_to_print[BUFFER_SIZE];
    char file_name[800];
    char file_path[BUFFER_SIZE];
    int file_size;
    int file_descriptor;

    // Get client name
    read_len = recv(socket, buffer, BUFFER_SIZE, 0);
    if (read_len <= 0)
    {
        // Client disconnected before sending the name
        close(socket);
        handle_client_disconnection(socket, NULL);
        return NULL;
    }

    buffer[read_len] = '\0';
    pthread_mutex_lock(&client_number_lock);
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
    pthread_mutex_unlock(&client_number_lock);
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
            memset(msg_to_print, 0, BUFFER_SIZE);
            memset(temp, 0, BUFFER_SIZE);

            strcat(msg_to_print, client_name);
            strcat(msg_to_print, ": ");

            if (strncmp(buffer, "TXT|", strlen("TXT|")) == 0)
            {
                char *buf_to_send;
                pthread_mutex_lock(&counter_lock);
                counter += 1;
                pthread_mutex_unlock(&counter_lock);
                int length = strlen("TXT|");
                memcpy(temp, buffer + length, strlen(buffer) - length + 1);
                strcat(msg_to_print, temp);
                printf("%d. %s\n", counter, msg_to_print);
                buf_to_send = malloc(length + strlen(msg_to_print));
                sprintf(buf_to_send, "%s%s", "TXT|", msg_to_print);
                send_to_all(socket, buf_to_send);
                continue;
            }
            else if (strncmp(buffer, "FILE|", strlen("FILE|")) == 0)
            {
                pthread_mutex_lock(&counter_lock);
                counter += 1;
                pthread_mutex_unlock(&counter_lock);
                sscanf(buffer, "%*[^|]|%d|%s|", &file_size, file_name);
                printf("\n%d. Recv file_name: %s from client: %s (sockfd: %d), file_size: %d\n", counter, file_name, client_name, socket, file_size);
                int total_bytes_recv = 0;
                int bytes_recv = 0;
                // sprintf(file_path, "./Server/%s", file_name);
                sprintf(file_path, "./%s", file_name);
                printf("file path: %s\n\n", file_path);

                recv_file(socket, file_size, file_path);
                memset(file_name, 0, sizeof(file_name));
                memset(file_path, 0, sizeof(file_path));
                continue;
            }
            else if (strncmp(buffer, "DOWN|", strlen("DOWN|")) == 0) // send file from server to client
            {
                pthread_mutex_lock(&counter_lock);
                counter += 1;
                pthread_mutex_unlock(&counter_lock);
                // msg format: DOWN|file_name
                sscanf(buffer, "%*[^|]|%s|", file_name);
                printf("\n%d. Request download file_name: %s from client %s (sockfd: %d)\n", counter, file_name, client_name, socket);
                // sprintf(file_path, "./Server/%s", file_name);
                sprintf(file_path, "./%s", file_name);

                printf("Opening file path: %s\n", file_path);
                send_file(socket, file_name, file_path);
                printf("\n");
                continue;
            }
        }
    } while (read_len > 0);

    // Client disconnected
    // printf("Client %s has closed the connection\n", client_name);

    // // Delete client from clients array
    // pthread_mutex_lock(&client_number_lock);
    // for (int j = 0; j < MAX_CLIENTS; j++)
    // {
    //     if (socket == clients[j].sockfd)
    //     {
    //         clients[j].sockfd = -1;
    //         free(clients[j].name);
    //         clients[j].name = NULL;
    //         break;
    //     }
    // }
    // pthread_mutex_unlock(&client_number_lock);
    handle_client_disconnection(socket, client_name);
    
    free(client_name);
    close(socket);
    return NULL;
}

void free_client_name(char *client_name)
{
    free(client_name);
}

void send_to_all(int socket, char *buffer)
{
    pthread_mutex_lock(&client_number_lock); // Protect clients array while sending
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sockfd != socket && clients[i].sockfd > 0 && clients[i].name != NULL)
        {
            send(clients[i].sockfd, buffer, strlen(buffer), 0);
        }
    }
    pthread_mutex_unlock(&client_number_lock);
}

char handle_client_disconnection(int socket, char *client_name)
{
    printf("Client %s has closed the connection\n", client_name);

    // Delete client from clients array and free client_name memory
    pthread_mutex_lock(&client_number_lock);
    for (int j = 0; j < MAX_CLIENTS; j++)
    {
        if (socket == clients[j].sockfd)
        {
            clients[j].sockfd = -1;
            free_client_name(clients[j].name);
            clients[j].name = NULL;
            current_client_number -= 1;
            break;
        }
    }
    pthread_mutex_unlock(&client_number_lock);
    close(socket);
}

void send_file(int socket, char *file_name, char *file_path)
{
    char buffer[BUFFER_SIZE];
    int file_size;

    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file");
        // send fail message to client with format: FILE|0|file_name
        memset(buffer, 0, BUFFER_SIZE);
        sprintf(buffer, "FILE|0|%s", file_name);
        send(socket, buffer, strlen(buffer), 0);
        return;
    }
    struct stat st;
    stat(file_path, &st);
    file_size = st.st_size;
    printf("file size: %d\n", file_size);

    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "FILE|%d|%s", file_size, file_name);
    // send file size
    send(socket, buffer, BUFFER_SIZE, 0);
    printf("buffer sent: %s\n", buffer);
    memset(buffer, 0, BUFFER_SIZE);

    int bytes_sent = 0;
    int total_bytes_sent = 0;
    while (total_bytes_sent < file_size)
    {
        // Read the file in chunks
        int bytes_read = read(fd, buffer, BUFFER_SIZE);
        if (bytes_read < 0)
        {
            perror("read");
            close(fd);
            printf("\n");
            continue;
        }

        // Send the chunk to the server
        bytes_sent = send(socket, buffer, bytes_read, 0);
        if (bytes_sent < 0)
        {
            perror("send");
            close(fd);
            continue;
        }

        total_bytes_sent += bytes_sent;
    }
    close(fd);
    // printf("\n");
}

void recv_file(int socket, int file_size, char *file_path)
{
    char buffer[BUFFER_SIZE];
    int total_bytes_recv;
    int bytes_recv;

    int fd = open(file_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (fd == -1)
    {
        perror("Error opening file");
        return;
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
        int bytes_written = write(fd, buffer, bytes_recv);

        if (bytes_written == -1)
        {
            perror("Error writing to file");
            close(fd);
            continue;
        }
        total_bytes_recv += bytes_recv;
        // printf("%s", buffer);
    }
    // printf("\ntotal bytes recv: %d\n", total_bytes_recv);
    close(fd);
    return;
}

int main(int argc, char const *argv[])
{
    // Init clients sockfd array
    // Init clients array
    clients = (Client *)malloc(MAX_CLIENTS * sizeof(Client));
    if (clients == NULL)
    {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
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
    if (pthread_mutex_init(&counter_lock, NULL) != 0)
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

        // Lock the client_number mutex to safely update client info and create a thread
        pthread_mutex_lock(&client_number_lock);
        // clients[client_number].sockfd = client_sockfd;
        for (int j=0; j < MAX_CLIENTS; j++)
        {
            if (clients[j].sockfd == -1 && clients[j].name == NULL)
            {
                clients[j].sockfd = client_sockfd;
                client_number = j;
                printf("client number:%d\n", client_number);
                break;
            }
        }
        // printf("Client has socketfd: %d has connected.\n\n", client_sockfd);

        // Create thread to handle each client
        if (pthread_create(&threads[client_number], NULL, connection_handle, (void *)&client_sockfd) != 0)
        {
            perror("pthread_create");
            pthread_mutex_unlock(&client_number_lock);
            exit(EXIT_FAILURE);
        }

        // client_number += 1;
        current_client_number += 1;
        printf("Current client number: %d\n", current_client_number);
        pthread_mutex_unlock(&client_number_lock);
    }

    // Destroy the mutexes
    pthread_mutex_destroy(&counter_lock);
    pthread_mutex_destroy(&client_number_lock);

    return 0;
}
