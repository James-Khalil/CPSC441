#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
int main(int argc, char *argv[])
  {
// Connecting to server
 struct sockaddr_in address;
 memset(&address, 0, sizeof(address));
 address.sin_family = AF_INET;
 address.sin_port = htons(atoi(argv[1]));
 address.sin_addr.s_addr = inet_addr("127.0.0.1");
 int listenSocket;
 listenSocket = socket(AF_INET, SOCK_STREAM, 0);
 if(listenSocket == -1){
 	printf("socket() call failed");
 }
 int status;
 status = connect(listenSocket, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
 if(status==-1){
 	printf("connect() call failed\n");
	close(listenSocket);
	exit(0);
 }

// Create the UCID and store it in a string
int count;
char UCID[9];
while(1){
printf("Provide a valid UCID:");
scanf("%s", UCID);
	if(strlen(UCID) >= 4 && strlen(UCID) < 9) break;
}

// Send the UCID to the server
count = send(listenSocket, UCID, 8, 0);
if(count == -1){
 	printf("send() call failed.");
}

// Recieve the datetime from the server
int servercount;
char datetime[100];
servercount = recv(listenSocket, datetime, 100, 0);
if(servercount == -1){
 	printf("recv() call failed.");
 }
 else{
	printf("Printing date time: ");
	printf(datetime);
	printf("\n");
 }

// Get last 4 digits from UCID
int UCIDint = atoi(UCID);
int last4 = UCIDint % 10000;

// Get seconds from datetime
char *token = strtok(datetime, ":");
    char *last_token = NULL;
    while (token != NULL) {
        last_token = token;
        token = strtok(NULL, ":");
    }

// Create passcode
int passcode = last4 + atoi(last_token);
printf("The passcode is %d\n", passcode);

// Send passcode to server
int passCount;
passCount = send(listenSocket, &passcode, 20, 0);
if(passCount == -1){
 	printf("send() call failed.");
}

// Connect to the output file
FILE *outputfile;
outputfile = fopen("recieved.txt", "w");
if (outputfile == NULL) {
	perror("Output file could not be accessed");
	return(1);
}

	char buffer[1024];
	while (recv(listenSocket, buffer, sizeof(buffer), 0) > 0)
	{
	int readBytes = strlen(buffer) - 1;
	if ((buffer[readBytes] != '\n')){
		readBytes++;
	}
	printf("%d Recieved from the Server!\n", readBytes);
	fputs(buffer,outputfile);
	memset(buffer, 0, sizeof(buffer));
	}



// Close the output file
fclose(outputfile);

// Close the socket
 close(listenSocket);
}
