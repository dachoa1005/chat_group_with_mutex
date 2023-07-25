#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

int main()
{
    const char *file_name = "example.txt";
    int file_descriptor;
    char client_input[1024];

    // Open the file for writing, create it if it doesn't exist, and set the permissions to rw-rw-rw-
    file_descriptor = open(file_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (file_descriptor == -1)
    {
        perror("Error opening file");
        return 1;
    }

    // Now, you can write to the file using the file descriptor.
    // For example, let's write a simple message to the file:
    const char *message = "Hello, this is a test message.\n";

    while (1)
    {
        fgets(client_input, 1024, stdin);
        // client_input[strlen(client_input) - 1] = '\0';
        ssize_t bytes_written = write(file_descriptor, client_input, strlen(client_input));

        if (bytes_written == -1)
        {
            perror("Error writing to file");
            close(file_descriptor);
            return 1;
        }
    }

    // Close the file when done writing.
    close(file_descriptor);

    return 0;
}
