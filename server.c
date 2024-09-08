#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main()
{

	char * ip= "127.0.0.1";
int port = 5566;
int server_sock,client_sock;
struct sockaddr_in server_addr,client_addr;
socklen_t addr_size;
char buffer[1024];
int n;
server_sock = socket(AF_INET, SOCK_STREAM,0);
if(server_sock<0){
perror("[-]socket error");
exit(1);
}
printf("[+] TCP server socket created");
memset(&server_addr,'\0',sizeof(server_addr));
server_addr.sin_family=AF_INET;
server_addr.sin_port = port;
server_addr.sin_addr.s_addr= inet_addr(ip);
n= bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr));
if(n<0){
perror("[-]Bind Error");
exit(1);
}
printf("[+] Binded to port number : %d\n",port);
listen(server_sock,5);
printf("Listening\n");
char * mess;
while(1){
    addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    printf("[+]Client connected.\n");

    bzero(buffer, 1024);
    recv(client_sock, buffer, sizeof(buffer), 0);
    printf("Client: %s\n", buffer);
    bzero(buffer, 1024);

   strcpy(buffer, "This Is Ack of Message ");
    printf("Server: %s\n", buffer);
    send(client_sock, buffer, strlen(buffer), 0);

    close(client_sock);
    printf("[+]Client disconnected.\n\n");

  }
return 0;
}
