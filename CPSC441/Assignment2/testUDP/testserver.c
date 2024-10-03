#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

int main(int argc, char *argv[]){
// Socket Creation
    int mySocket;
    mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(mySocket == -1){
        printf("Could not setup a socket\n");
        }
    else printf("Socket successfully setup\n");

    // Binding
    int status;
    struct sockaddr_in ip_server;
    struct sockaddr *server;
    memset ((char*) &ip_server, 0, sizeof(ip_server));
    ip_server.sin_family = AF_INET;
    ip_server.sin_port = htons(atoi(argv[1]));
    ip_server.sin_addr.s_addr = htonl(INADDR_ANY);
    server = (struct sockaddr *) &ip_server;
    status = bind(mySocket, server, sizeof(ip_server));
    if(status == -1){
        printf("Could not bind to port\n");
        return -1;
        }
    else printf("Binding successfully setup\n");
    printf("Waiting for message from client\n");
  // Authentication
    // Recieve AUTH
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);


    char messagein[32];
    int readbytes;
    readbytes = recvfrom(mySocket, messagein, 
    32, 0, 
    (struct sockaddr *)&client_address, &client_address_len);
    if ( readbytes<0){
        printf("Read error\n");
        return -1;
        }
    printf("read %d bytes that said %s\n", readbytes, messagein);

    char SendBuff[32] = "returned ACK";
    int num_bytes = sendto(mySocket, SendBuff, strlen(SendBuff), 0, (struct sockaddr *)&client_address,
     sizeof(client_address));
    if(num_bytes == -1) printf("Unsuccessful send\n");
    else printf("number of sent bytes = %d\n",num_bytes);
    return 0;
}