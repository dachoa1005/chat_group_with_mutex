#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

// pthread_mutex_t socket_lock;

void *send_message(void *client_sockfd);
void *recv_message(void *client_sockfd);
void send_msg_to_serv(int socket, char *client_input);
void send_file(int socket, char *file_path);
void down_file(int socket, int file_size, char *file_path);

void *send_message(void *client_sockfd)
{
    int socket = *(int *)client_sockfd;
    char client_name[100];
    srand(time(NULL) + getpid() + 1);
    int random_num = rand() % 1000;
    char buffer[1024];

    // enter name and check then send to server
    sprintf(client_name, "%d", random_num);
    // usleep(100000);

    if (send(socket, client_name, BUFFER_SIZE, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    printf("%s Connected to server\n", client_name);
    // usleep(15000000); // sleep 30s
    int i = 0;
    while (i < 10)
    {
        usleep(500000);
        i += 1;
        sprintf(buffer, "TXT|Message number %d from %s", i, client_name);
        buffer[strlen(buffer)] = '\0';
        send(socket, buffer, BUFFER_SIZE, 0);
        fflush(stdout);
        memset(buffer, 0, BUFFER_SIZE);
        usleep(500000);
    }
}

void *recv_message(void *client_sockfd)
{
    int socket = *(int *)client_sockfd;
    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE];
    int file_size = 0;
    char file_name[800];
    char file_path[BUFFER_SIZE];

    while (1)
    {
        usleep(1000);
        memset(buffer, 0, BUFFER_SIZE);
        memset(msg, 0, BUFFER_SIZE);
        // pthread_mutex_lock(&socket_lock);
        int recv_len = recv(socket, buffer, BUFFER_SIZE, 0);
        // pthread_mutex_unlock(&socket_lock);
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
        // buffer[recv_len] = '\0';
        buffer[strlen(buffer)] = '\0';

        // check if message is a txt or file? (format: TXT|message or FILE|file_size|data)
        if (strncmp(buffer, "TXT|", strlen("TXT|")) == 0) // this is a Text
        {
            // sscanf(buffer, "%*s|%s", msg);
            memcpy(msg, buffer + strlen("TXT|"), strlen(buffer) - strlen("TXT|") + 1);
            // printf("%s\n", msg);
            continue;
        }
        else if (strncmp(buffer, "FILE|", strlen("FILE|")) == 0)
        {
            sscanf(buffer, "%*[^|]|%d|%s|", &file_size, file_name);
            printf("file size: %d, file name: %s\n", file_size, file_name);
            if (file_size == 0) // file not exist
            {
                printf("File not exist\n");
                continue;
            }
            else if (file_size > 0)
            {
                // sprintf(file_path, "./Client/%s", file_name);
                sprintf(file_path, "./%s", file_name);
                printf("file path: %s\n", file_path);

                down_file(socket, file_size, file_path);
                continue;
            }
        }
    }
}

const char *get_file_name(const char *path)
{
    // Find the last occurrence of the path separator '/'
    const char *last_slash = strrchr(path, '/');

    // If the last_slash is not NULL, the file name starts after the slash,
    // otherwise, the entire path is the file name.
    return last_slash ? last_slash + 1 : path;
}

void send_file(int socket, char *file_path)
{
    int bytes_read;
    char buffer[BUFFER_SIZE];

    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        perror("open fail");
        return;
    }

    const char *file_name = get_file_name(file_path);

    struct stat st;
    stat(file_path, &st);
    int file_size = st.st_size;
    printf("file size: %d\n", file_size);

    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "FILE|%d|%s", file_size, file_name);
    // send file_size
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
            return;
        }

        // Send the chunk to the server
        bytes_sent = send(socket, buffer, bytes_read, 0);
        if (bytes_sent < 0)
        {
            perror("send");
            close(fd);
            return;
        }

        total_bytes_sent += bytes_sent;
    }
    close(fd);
}

void down_file(int socket, int file_size, char *file_path)
{
    char buffer[BUFFER_SIZE];
    int file_descriptor = open(file_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (file_descriptor == -1)
    {
        perror("Error opening file");
        return;
    }
    int total_bytes_recv = 0;
    int bytes_recv = 0;
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
    close(file_descriptor);
    return;
}

void send_msg_to_serv(int socket, char *client_input)
{
    // char *msg = malloc(sizeof("TXT|")+ sizeof(client_input));
    char msg[BUFFER_SIZE];
    sprintf(msg, "%s%s", "TXT|", client_input);
    if (send(socket, msg, BUFFER_SIZE, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    return;
}

int main(int argc, char *argv[])
{
    pthread_t send_thread, recv_thread;
    // if (pthread_mutex_init(&socket_lock, NULL) != 0)
    //     {
    //         perror("pthread_mutex_init");
    //         exit(EXIT_FAILURE);
    //     }
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

    // printf("Connected to server\n");

    pthread_create(&send_thread, NULL, send_message, (void *)&client_sockfd);
    pthread_create(&recv_thread, NULL, recv_message, (void *)&client_sockfd);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    return 0;
}