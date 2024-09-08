#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#define BUFFER_SIZE 2048
#define ENCRYPTION_KEY 0x0B
#define MAX_FILE_NAME 255
#define EXT_SIZE      4
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

// Function to decode an encoded message using the key
void decode(char *encoded_message)
{
    int len = strlen(encoded_message);
    for (int i = 0; i < len; i++)
    {
        encoded_message[i] = encoded_message[i] ^ ENCRYPTION_KEY;
    }
    encoded_message[len] = '\0';
}
int send_to_server(int sockfd, void *buffer, int size)
{
    int n1=0,n=0;	//Write to server
    while(n1<size)
    {
        n=write(sockfd, buffer+n1, size-n1);
        n1+=n;
    }
    if (n < 0)
    {
        perror("Error writing to server");
        free(buffer);
    }
}
void *receive_messages(void *client)
{   int  client_socket=(int)client;
    // Receive and print response from server
    char response[BUFFER_SIZE];
    while(1)
    {
        int bytes_received = recv(client_socket, response, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            printf("Server disconnected\n");
            break;
        }
        response[bytes_received] = '\0';
        decode(response);
        if(strncmp(response, "file:", strlen("file:")) == 0)
        {
            char message[BUFFER_SIZE];
            sscanf(response,"file:%s",message);
            printf("\n%s\n", message);
            write_file(client_socket);
            memset(response,0,sizeof(response));
        }
        else
        {
            printf("\n%s\n", response);
        }
    }
    return NULL;
}
int read_image_file(char *file_name, char **buffer, int *sizeof_buffer)
{
    int c, i;
    int char_read = 0;
    FILE* fp = fopen(file_name, "rb");

    if (fp == NULL)
    {
        printf("Can't open file : %s", file_name);
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    *sizeof_buffer = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    *buffer = (char *)malloc(*sizeof_buffer);
    fread(*buffer, *sizeof_buffer, 1, fp);
    return 0;
}
void write_file(int sockfd)
{
    char *buffer    = NULL;
    int buffer_size = 0,n;
    char extension[4];
    char file_name[256];
    FILE* fp = NULL;
    n = read(sockfd, &buffer_size, sizeof(int));//Read incoming data streams

    if(n < 0)
        perror("Error reading size from Client");
    printf("\nFile Received of Size : %d\n", buffer_size);
    buffer = malloc(buffer_size);
    n = read(sockfd, extension, 4);//Read incoming data streams
    if(n < 4)
        perror("Error reading extension from Client");
    strcpy(file_name, "new-file.");
    strcat(file_name, extension);
    fp = fopen(file_name, "wb");
    if (fp == NULL)
    {
        perror("Error opening file");
    }
    int tot=0;
    n=0;
    while(tot<buffer_size)
    {

        n = read(sockfd, buffer, buffer_size);
        tot+=n;
        fwrite(buffer, n, 1, fp);
        bzero(buffer,buffer_size);
    }
    if(tot< buffer_size)
        perror("Error reading file from Client");

    fclose(fp);

}
// Function to send a file to the server
void send_file(int sockfd)
{
    int n;
    char file_name[MAX_FILE_NAME];
    char extension[EXT_SIZE];
    int size_of_file = 0;
    char *buffer     = NULL;
    printf("\tPlease enter a image to send : ");
    scanf("%s", &file_name);
    printf("\tExtension : ");
    scanf("%s", &extension);

    if (read_image_file(file_name, &buffer, &size_of_file))
    {
        perror("Reading Image Failed");
        if (n < 0)
        {
            if (sockfd > 0)
                close(sockfd);
        }
    }
    printf("\tSize of file : %d\n", size_of_file);
    n = send_to_server(sockfd, &size_of_file, sizeof(int));
    if (n < 0)
    {
        if (sockfd > 0)
            close(sockfd);
    }
    n = send_to_server(sockfd, extension, EXT_SIZE);
    if (n < 0)
    {
        if (sockfd > 0)
            close(sockfd);
    }
    n = send_to_server(sockfd, buffer, size_of_file);
    if (n < 0)
    {
        if (sockfd > 0)
            close(sockfd);
    }

}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port> <nickname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_addr;

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
    char nickname[BUFFER_SIZE];
    strcpy(nickname, argv[3]);
    encode(nickname);
    send(client_socket, nickname, strlen(nickname), 0);

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)client_socket) != 0)
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    // Start sending messages or files
    while (1)
    {
        printf("Enter message : ");
        char input[BUFFER_SIZE];
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "exit") == 0)
        {
            printf("Exiting...\n");
            break;
        }
        if (strncmp(input, "file:", strlen("file:")) == 0)
        {
            encode(input);
            send(client_socket, input, strlen(input), 0);
            send_file(client_socket);
            continue;
        }
        else
        {
            encode(input);
            send(client_socket, input, strlen(input), 0);
        }
    }

    // Close client socket
    close(client_socket);
    //join the thread
    pthread_join(receive_thread, NULL);

    return 0;
}
