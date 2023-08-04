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
#include <netinet/tcp.h>
#include <signal.h>

#define MAX_CLIENTS 10000
#define BUFFER_SIZE 1024
#define FILE_NAME_SIZE 500
#define FILE_PATH_SIZE 1000
int counter;
pthread_mutex_t counter_lock;
pthread_mutex_t socket_lock;
pthread_mutex_t clients_lock[MAX_CLIENTS];
pthread_mutex_t client_sockfd_lock;
pthread_mutex_t client_number_lock;

typedef struct
{
    int sockfd;
    char *name;
} Client;

Client *clients;
int client_index = 0;
int current_client_number = 0;

void send_file(int socket, char *file_name, char *file_path);
void recv_file(int socket, int file_size, char *file_path);
void handle_client_disconnection(int socket, char *client_name);
void send_to_all(int socket, char *buffer);
void *connection_handle(void *arg);
void recv_client_name(int socket, char **client_name);
void handle_message(int socket, const char *buffer, char *msg_to_print);
void handle_upload(int socket, const char *buffer, const char *client_name);
void handle_download(int socket, const char *buffer, const char *client_name);

void *connection_handle(void *arg)
{
    int socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char *client_name;
    int read_len = 0;
    char msg_to_print[BUFFER_SIZE];

    // Get client name
    recv_client_name(socket, &client_name);

    do
    {
        memset(buffer, 0, BUFFER_SIZE);
        read_len = recv(socket, buffer, BUFFER_SIZE, 0);
        if (read_len <= 0)
        {
            handle_client_disconnection(socket, client_name);

            // free(client_name);
            // close(socket);
        }
        if (strcmp(buffer, "") == 0)
            continue;

        // buffer[read_len] = '\0';
        // printf("buffer: %s\n", buffer);

        if (read_len > 0)
        {
            memset(msg_to_print, 0, BUFFER_SIZE);
            strcat(msg_to_print, client_name);
            strcat(msg_to_print, ": ");

            if (strncmp(buffer, "TXT|", strlen("TXT|")) == 0)
            {
                buffer[strlen(buffer)] = '\0';
                handle_message(socket, buffer, msg_to_print);
            }
            else if (strncmp(buffer, "FILE|", strlen("FILE|")) == 0)
            {
                handle_upload(socket, buffer, client_name);
            }
            else if (strncmp(buffer, "DOWN|", strlen("DOWN|")) == 0)
            {
                handle_download(socket, buffer, client_name);
            }
        }
    } while (read_len > 0);
    return NULL;
}

void handle_message(int socket, const char *buffer, char *msg_to_print)
{
    char temp[BUFFER_SIZE];
    char *buf_to_send;
    pthread_mutex_lock(&counter_lock);
    counter += 1;
    int length = strlen("TXT|");
    memcpy(temp, buffer + length, strlen(buffer) - length + 1);
    strcat(msg_to_print, temp);
    buf_to_send = malloc(length + strlen(msg_to_print));
    sprintf(buf_to_send, "%s%s", "TXT|", msg_to_print);
    buf_to_send[strlen(buf_to_send)] = '\0';
    send_to_all(socket, buf_to_send);
    printf("%d. %s\n", counter, msg_to_print);
    pthread_mutex_unlock(&counter_lock);
    // memset(buffer, 0, BUFFER_SIZE);
    memset(temp, 0, BUFFER_SIZE);
    free(buf_to_send);
    // memset(buf_to_send, 0, BUFFER_SIZE);
}

void handle_upload(int socket, const char *buffer, const char *client_name)
{

    int file_size;
    char file_name[FILE_NAME_SIZE];
    sscanf(buffer, "%*[^|]|%d|%s|", &file_size, file_name);
    printf("\n%d. Recv file_name: %s from client: %s (sockfd: %d), file_size: %d\n", counter, file_name, client_name, socket, file_size);
    // int total_bytes_recv = 0;
    // int bytes_recv = 0;
    char file_path[FILE_PATH_SIZE];
    sprintf(file_path, "./%s", file_name);
    printf("file path: %s\n\n", file_path);
    
    recv_file(socket, file_size, file_path);
    memset(file_name, 0, sizeof(file_name));
    memset(file_path, 0, sizeof(file_path));
}

void handle_download(int socket, const char *buffer, const char *client_name)
{
    pthread_mutex_lock(&counter_lock);
    counter += 1;
    pthread_mutex_unlock(&counter_lock);
    char file_name[FILE_NAME_SIZE];
    // msg format: DOWN|file_name
    sscanf(buffer, "%*[^|]|%s|", file_name);
    printf("\n%d. Request download file_name: %s from client %s (sockfd: %d)\n", counter, file_name, client_name, socket);
    char file_path[FILE_PATH_SIZE];
    sprintf(file_path, "./%s", file_name);

    printf("Opening file path: %s\n", file_path);
    send_file(socket, file_name, file_path);
    printf("\n");
}

void recv_client_name(int socket, char **client_name)
{
    char buffer[BUFFER_SIZE];
    int read_len = 0;

    read_len = recv(socket, buffer, BUFFER_SIZE, 0);
    if (read_len <= 0)
    {
        // Client disconnected before sending the name
        handle_client_disconnection(socket, NULL);

        // close(socket);
        pthread_exit(NULL);
        return;
    }

    buffer[strlen(buffer)] = '\0';
    pthread_mutex_lock(&client_number_lock);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sockfd == socket)
        {
            pthread_mutex_lock(&clients_lock[i]);
            clients[i].name = malloc(strlen(buffer) + 1);
            strcpy(clients[i].name, buffer);
            *client_name = malloc(strlen(buffer) + 1); // Update the value of client_name using a double pointer
            strcpy(*client_name, buffer);              // Dereference the double pointer to modify the value it points to
            pthread_mutex_unlock(&clients_lock[i]);
            break;
        }
    }
    pthread_mutex_unlock(&client_number_lock);
    printf("Client %s has joined the chat\n", *client_name); // Dereference the double pointer to print the updated value
}

void free_client_name(char *client_name)
{
    free(client_name);
}

void send_to_all(int socket, char *buffer)
{
    pthread_mutex_lock(&client_number_lock); // Protect clients array while sending
    // printf("Sending to all clients: %s\n", buffer);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sockfd != socket && clients[i].sockfd > 0 && clients[i].name != NULL)
        {
            // pthread_mutex_lock(&clients_lock[i]);
            int bytes_sent = send(clients[i].sockfd, buffer, BUFFER_SIZE, 0);
            if (bytes_sent == -1)
                {
                    // perror("sent");
                    continue;    
                }

            // pthread_mutex_unlock(&clients_lock[i]);
        }
    }
    pthread_mutex_unlock(&client_number_lock);
}

void handle_client_disconnection(int socket, char *client_name)
{
    if (client_name != NULL)
        printf("Client %s has closed the connection\n", client_name);
    else
        printf("Client has sockfd: %d has closed the connection\n", socket);
    // Delete client from clients array and free client_name memory
    // pthread_mutex_lock(&client_number_lock);
    for (int j = 0; j < MAX_CLIENTS; j++)
    {
        if (socket == clients[j].sockfd)
        {
            pthread_mutex_lock(&clients_lock[j]);
            clients[j].sockfd = -1;
            free_client_name(clients[j].name);
            clients[j].name = NULL;
            current_client_number -= 1;
            pthread_mutex_unlock(&clients_lock[j]);
            break;
        }
    }
    // pthread_mutex_unlock(&client_number_lock);
    // Close client's socket
    if (client_name != NULL)
        free(client_name);
    close(socket);
    pthread_exit(NULL);
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
        send(socket, buffer, BUFFER_SIZE, 0);
        return;
    }

    // get file size
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
}

void recv_file(int socket, int file_size, char *file_path)
{
    char buffer[BUFFER_SIZE];
    int total_bytes_recv = 0;
    int bytes_recv = 0;

    int fd = open(file_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (fd == -1)
    {
        perror("Error opening file");
        return;
    }
    while (total_bytes_recv < file_size)
    {
        memset(buffer, 0, BUFFER_SIZE);
        pthread_mutex_lock(&counter_lock);
        counter += 1;
        pthread_mutex_unlock(&counter_lock);
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
        printf("%d. Recv total %d\n", counter, total_bytes_recv);
    }
    close(fd);
    return;
}

void init_clients_array()
{
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
}

void init_mutex_lock()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (pthread_mutex_init(&clients_lock[i], NULL) != 0)
        {
            perror("pthread_mutex_init");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_init(&socket_lock, NULL) != 0)
    {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }

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
    return;
}

void handle_sigpipe(int signal) {
    // Xử lý tín hiệu SIGPIPE ở đây
    // printf("Nhận được tín hiệu SIGPIPE, xử lý hành vi thích hợp...\n");
}

int main(int argc, char const *argv[])
{
    printf("PID: %d\n", getpid());
    int port = atoi(argv[1]);
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_address;
    int addrlen = sizeof(server_address);
    int optval = 1;
    // char buffer[BUFFER_SIZE] = {0};
    pthread_t threads[MAX_CLIENTS];

    signal(SIGPIPE, handle_sigpipe);

    // Init clients sockfd array
    init_clients_array();

    // Init mutex locks
    init_mutex_lock();

    if (argc < 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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

    if (setsockopt(server_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == -1) {
        perror("Khong the set option TCP_NODELAY");
        return 1;
    }

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

    printf("Listening on port %d\n", port);
    while (1)
    {
        usleep(10000);
        if (current_client_number >= MAX_CLIENTS) {
            printf("Server has reach max clients, waiting...\n");
            sleep(1); 
            continue;
        }
        // Accept connection from client
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&server_address, (socklen_t *)&addrlen);
        if (client_sockfd < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // int enable = 1;
        // setsockopt(client_sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

        // Lock the client number mutex to safely update client info and create a thread
        pthread_mutex_lock(&client_number_lock);
        // for (int j = 0; j < MAX_CLIENTS; j++)
        // {
        //     if (clients[j].sockfd == -1 && clients[j].name == NULL)
        //     {
        //         pthread_mutex_lock(&clients_lock[j]);
        //         clients[j].sockfd = client_sockfd;
        //         client_index = j;
        //         printf("client index:%d\n", client_index);
        //         pthread_mutex_unlock(&clients_lock[j]);
        //         break;
        //     }
        // }
        // while (clients[client_index].sockfd != -1 && clients[client_index].name != NULL)
        // {
        //     client_index += 1;
        //     if (client_index == MAX_CLIENTS)
        //         client_index = 0;
        // }

        while (client_index < MAX_CLIENTS)
        {
            if (clients[client_index].sockfd == -1 && clients[client_index].name == NULL)
                break;
            else
                client_index += 1;
        }
        
        // while (current_client_number >= MAX_CLIENTS)
        //     {
        //         // printf
        //         sleep(10);
        //     }
        // reset index to 0 if index > MAX_CLIENTS
        if (client_index >= MAX_CLIENTS)
            client_index = 0;
        // add client to clients array
        // pthread_mutex_lock(&clients_lock[client_index]);
        clients[client_index].sockfd = client_sockfd;
        printf("client index:%d\n", client_index);
        // pthread_mutex_unlock(&clients_lock[client_index]);

        // printf("Client has socketfd: %d has connected.\n\n", client_sockfd);

        // Create thread to handle each client
        if (pthread_create(&threads[client_index], NULL, connection_handle, (void *)&clients[client_index].sockfd) != 0)
        {
            perror("pthread_create");
            pthread_mutex_unlock(&client_number_lock);
            // exit(EXIT_FAILURE);
            continue;
        }

        client_index += 1;
        current_client_number += 1;
        
        printf("Current client number: %d\n", current_client_number);
        pthread_mutex_unlock(&client_number_lock);
    }

    // Destroy the mutexes
    pthread_mutex_destroy(&counter_lock);
    pthread_mutex_destroy(&client_number_lock);

    return 0;
}
