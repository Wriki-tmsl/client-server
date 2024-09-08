#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define ENCRYPTION_KEY 0x0B
// Function to encode a message using the key
void encode(char *message)
{
    int len = strlen(message);
    for (int i = 0; i < len; i++)
    {
        message[i] = message[i] ^ ENCRYPTION_KEY;
    }
    message[len] = '\0';
}

// Function to decode an encoded message using key
void decode(char *encoded_message)
{
    int len = strlen(encoded_message);
    for (int i = 0; i < len; i++)
    {
        encoded_message[i] = encoded_message[i] ^ ENCRYPTION_KEY;
    }
    encoded_message[len] = '\0';
}
void send_file(int client_socket, const char *file_name)
{
    char hello[10]="file:";
    encode(hello);
    send(client_socket, header, strlen(header), 0);
    FILE *file = fopen(file_name, "rb");
    if (!file)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // Calculate file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    // Send file header containing file size
    char header[BUFFER_SIZE];
    snprintf(header, BUFFER_SIZE, "%ld", file_size);
    encode(header);
    send(client_socket, header, strlen(header), 0);
    // Wait for acknowledgement from the server
    char ack[BUFFER_SIZE];
    recv(client_socket, ack, BUFFER_SIZE, 0);
    printf("Starting file Transfer\n");
    // Send file contents
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        encode(buffer);
        send(client_socket, buffer, bytes_read, 0);
    }
    fclose(file);
    // Wait for acknowledgement of completion from the server
    recv(client_socket, ack, BUFFER_SIZE, 0);
    printf("Completed File Transfer\n");
}
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE], nickname[BUFFER_SIZE];

    // Create client socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%s\n", argv[1], argv[2]);

    // Send nickname to server
    printf("Enter your nickname: ");
    fgets(nickname, BUFFER_SIZE, stdin);
    nickname[strcspn(nickname, "\n")] = '\0';
    encode(nickname);
    send(client_socket, nickname, strlen(nickname), 0);

    // Start receiving messages from server
    while (1)
    {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        encode(buffer);
        if (strcmp(buffer, "exit") == 0)
        {
            printf("Exiting...\n");
            break;
        }
        if (strncmp("file:",buffer,sizeof("file:"))==0)
        {
            char *recipient = strtok(buffer + 1, ":");
            char *file_name = strtok(NULL, "");
            send_file(client_socket,file_name);
        }

        // Send message to server
        if (send(client_socket, buffer, strlen(buffer), 0) == -1)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
        // Check for exit command
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        else if (bytes_received == 0)
        {
            printf("Server disconnected\n");
            break;
        }
        else
        {
            buffer[bytes_received] = '\0';
            decode(buffer);
            printf("%s\n", buffer);
        }
        memset(buffer,0,BUFFER_SIZE);
    }

    close(client_socket);
    return 0;
}

