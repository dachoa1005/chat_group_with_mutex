#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

void printf_screen(char *client_input)
{
    char *msg = malloc(sizeof("TXT|")+ sizeof(client_input));
    sprintf(msg, "%s%s", "TXT|", client_input);
    printf("%s\n", msg);
}

int main(int argc, char const *argv[])
{
    char client_input[1024];
    char file_path[1024];
    char temp[1024];
    char temp2[1024];


    while (1)
    {
        memset(client_input, 0, 1024);
        memset(temp, 0, sizeof(temp));
        // memset(file_path, 0, sizeof(file_path));
        printf("Enter client_input: ");
        fgets(client_input, 1024, stdin);
        client_input[strlen(client_input)-1]='\0';
        sscanf(client_input, "%s|%s", temp, temp2);
        printf("temp: %s\n",temp);
        printf_screen(client_input);
    }
    return 0;
}
