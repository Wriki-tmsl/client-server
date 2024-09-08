#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
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
typedef struct
{
    int socket;
    char nickname[BUFFER_SIZE];
} Client;
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
void write_file(int sockfd,int dest_sock)
{
    char *buffer    = NULL;
    int buffer_size = 0,n;
    char extension[4];
    char file_name[256];
    FILE* fp = NULL;
    n = read(sockfd, &buffer_size, sizeof(int));//Read incoming data streams
    if(n < 0)
        perror("Error reading size from Client");
    printf("%d", buffer_size);
    if (n < 0)
    {
        if (sockfd > 0)
            close(sockfd);
    }
    buffer = malloc(buffer_size);
    n = read(sockfd, extension, 4);//Read incoming data streams
    if(n < 4)
        perror("Error reading extension from Client");
    strcpy(file_name, "new-file");
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
    send_file(file_name,dest_sock,extension);

}
void send_file(char * file_name,int sockfd,char* extension)
{
    int n;
    //char file_name[MAX_FILE_NAME];
    //char extension[EXT_SIZE];
    int size_of_file = 0;
    char *buffer     = NULL;
    //printf("\t Please enter a image to send : ");
    //scanf("%s", &file_name);
    //printf("\t Extension : ");
    //scanf("%s", &extension);
    if (read_image_file(file_name, &buffer, &size_of_file))
    {
        perror("Reading Image Failed");
        if (n < 0)
        {
            if (sockfd > 0)
                close(sockfd);
        }
    }
    printf("\t Size of file : %d\n", size_of_file);
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
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_socket, client_count = 0, max_socket;
    Client client_sockets[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    fd_set read_fds;
    char buffer[BUFFER_SIZE];

    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    // Bind server socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Chat server started on port %s\n", argv[1]);

    max_socket = server_socket;

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        for (int i = 0; i < client_count; i++)
        {
            FD_SET(client_sockets[i].socket, &read_fds);
            if (client_sockets[i].socket > max_socket)
            {
                max_socket = client_sockets[i].socket;
            }
        }

        // Wait for activity on any of the sockets
        if (select(max_socket + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check for incoming connection
        if (FD_ISSET(server_socket, &read_fds))
        {
            int new_socket;
            socklen_t addr_len = sizeof(client_addr);

            // Accept new connection
            if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) == -1)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            // Receive nickname from client
            int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0)
            {
                close(new_socket);
                continue;
            }
            buffer[bytes_received] = '\0';
            decode(buffer);
            char notification[BUFFER_SIZE];
            snprintf(notification, BUFFER_SIZE, "Welcome to the chat %s !\nTo send Private message Type @recipient:messege\nTo send file type file:recipient\nKeep Chatting !!\n", buffer);
            encode(notification);
            send(new_socket,notification, BUFFER_SIZE, 0);
            printf("New connection from %s:%d with nicknanme : %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),buffer);
            memset(notification,0,BUFFER_SIZE);
            snprintf(notification, BUFFER_SIZE, "%s has joined the chat\n", buffer);
            encode(notification);
            for (int j = 0; j < client_count; j++)
            {
                send(client_sockets[j].socket, notification, strlen(notification), 0);
            }
            // Add new client to the list
            if (client_count < MAX_CLIENTS)
            {
                client_sockets[client_count].socket = new_socket;
                strcpy(client_sockets[client_count].nickname, buffer);
                client_count++;
            }
            else
            {
                printf("Too many clients. Connection rejected.\n");
                close(new_socket);
            }
        }

        // Check for activity on client sockets
        for (int i = 0; i < client_count; i++)
        {
            int client_socket = client_sockets[i].socket;

            if (FD_ISSET(client_socket, &read_fds))
            {
                int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
                char noti[BUFFER_SIZE];
                if (bytes_received <= 0)
                {
                    // Connection closed or error
                    if (bytes_received == 0)
                    {
                        snprintf(noti,BUFFER_SIZE,"Client %s disconnected\n", client_sockets[i].nickname);
                        encode(noti);
                        for (int j = 0; j < client_count; j++)
                        {
                            int dest_socket = client_sockets[j].socket;
                            if (dest_socket != client_socket)
                            {
                                send(client_sockets[j].socket, noti, strlen(noti), 0);
                            }
                        }
                    }
                    else
                    {
                        perror("recv");
                    }
                    close(client_socket);
                    // Remove client from the list
                    if (i < client_count - 1)
                    {
                        client_sockets[i] = client_sockets[client_count - 1];
                    }
                    client_count--;
                }
                else
                {
                    buffer[bytes_received] = '\0';
                    decode(buffer);
                    if (strncmp(buffer, "@", 1) == 0)
                    {
                        // Private message format: @recipient:message
                        char *recipient = strtok(buffer + 1, ":");
                        char *message = strtok(NULL, "");
                        char private_message[BUFFER_SIZE];
                        snprintf(private_message, BUFFER_SIZE, "Private [%s] : %s\n",  client_sockets[i].nickname, message);
                        encode(private_message);
                        for (int j = 0; j < client_count; j++)
                        {

                            if (strcmp(client_sockets[j].nickname,recipient)==0)
                            {
                                send(client_sockets[j].socket, private_message, strlen(private_message), 0);
                            }
                        }

                    }
                    else if (strncmp(buffer, "file:", strlen("file:")) == 0)
                    {

                        char name[50];
                        sscanf(buffer, "file:%s", name);
                        for (int j = 0; j < client_count; j++)
                        {

                            if (strcmp(client_sockets[j].nickname,name)==0)
                            {   //encode(buffer);
                                char message[BUFFER_SIZE];
                                snprintf(message, BUFFER_SIZE, "file:Private image from [%s]\n",  client_sockets[i].nickname);
                                encode(message);
                                send(client_sockets[j].socket,message,strlen(message),0);
                                write_file(client_socket,client_sockets[j].socket);
                            }
                        }

                        //write_file(client_socket);

                        memset(buffer,0,sizeof(buffer));

                    }
                    else
                    {
                        // Broadcast received message to all clients
                        char message[BUFFER_SIZE];
                        snprintf(message, BUFFER_SIZE, "[%s] : %s \n", client_sockets[i].nickname, buffer);
                        encode(message);
                        printf("%s\n",message);
                        for (int j = 0; j < client_count; j++)
                        {
                            int dest_socket = client_sockets[j].socket;

                            if (dest_socket != client_socket)
                            {
                                send(dest_socket, message, strlen(message), 0);
                            }
                        }
                    }
                }
                memset(buffer,0,sizeof(buffer));
            }
        }
    }

    close(server_socket);
    return 0;
}
