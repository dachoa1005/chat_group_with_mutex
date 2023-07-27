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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

pthread_mutex_t lock;

void *send_message(void *client_sockfd);
void *recv_message(void *client_sockfd);
void send_msg_to_serv(int socket, char *client_input);
void send_file(int socket, char *file_path);
void down_file(int socket, int file_size, char *file_path);

void *send_message(void *client_sockfd)
{
    int socket = *(int *)client_sockfd;
    srand(time(NULL));
    int random_num = rand() % 1000;
    char client_name[100];

    // enter name and check then send to server
    sprintf(client_name, "%d", random_num);

    if (send(socket, client_name, sizeof(client_name), 0) < 0)
    {
        perror("send");
        exit(1);
    }

    char client_input[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    char file_name[800];
    char buffer[BUFFER_SIZE];
    for (int i = 0; i < 100; i++)
    {
        usleep(25000);
        char message[BUFFER_SIZE];
        sprintf(message, "TXT|Message %d", i + 1);
        send(socket, message, strlen(message)+1, 0);
        printf("%s msg: %d\n", client_name, i);
        usleep(25000);
    }
    printf("%s done \n", client_name);

}

void send_msg_to_serv(int socket, char *client_input)
{
    // char *msg = malloc(sizeof("TXT|")+ sizeof(client_input));
    char msg[BUFFER_SIZE];
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
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
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
    // pthread_create(&recv_thread, NULL, recv_message, (void *)&client_sockfd);

    pthread_join(send_thread, NULL);
    // pthread_join(recv_thread, NULL);

    return 0;
}