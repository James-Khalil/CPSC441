#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
int main(int argc, char *argv[])
{
	// Sort information as required
    char *info = argv[1];
    char *token = strtok(info, ":");
    char *username = token;
    token = strtok(NULL, " ");
    char *password = token;

// Connecting to client
 struct sockaddr_in address;
 memset(&address, 0, sizeof(address));
 address.sin_family = AF_INET;
 address.sin_port = htons(atoi(argv[1]));
 address.sin_addr.s_addr = htonl(INADDR_ANY);
 int listenSocket;
 listenSocket = socket(AF_INET, SOCK_STREAM, 0);
 if(listenSocket == -1){
 	printf("socket() call failed");
 }
 int status;
 status = bind(listenSocket, (struct sockaddr *)
 &address, sizeof(struct sockaddr_in));
 if(status==-1){
 	printf("bind() call failed");
}
 status = listen(listenSocket,5);
 if(status==-1){
 	printf("listen() call failed");
 }

while(1) // While true
{
	int acceptSocket;
	acceptSocket = accept(listenSocket, NULL, NULL);
	if(acceptSocket == -1){
	printf("accept() call failed");
	}

	// Recieve the UCID from the client
	int count;
	char UCID[9];
	recv(acceptSocket, UCID, sizeof(UCID), 0);
	printf("The UCID given was %s\n", UCID);

	// Create the datetime
	time_t rawtime = time(NULL);
	// struct tm * timeinfo;
	struct tm tm = *localtime(&rawtime);
	int seconds = tm.tm_sec;

	char datetime[100];
	sprintf(datetime, "%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, 
	tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	// Send the datetime
	send(acceptSocket, datetime, 100, 0);

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

	// Read passcode from client
	int passCount;
	int clientPass;
	passCount = recv(acceptSocket, &clientPass, 20, 0);
	if(passCount == -1)
	{
		printf("recv() call failed.");
	}

	// If passcodes match, open data.txt
	if(clientPass == passcode)
	{

		// Get the input file data.txt
		FILE *inputfile;
		char buffer[1024];

		inputfile = fopen("data.txt", "r");
		if (inputfile == NULL) 
		{
			perror("Input file could not be accessed");
			return(0);
		}

		// Send the data.txt line by line
		while (fgets(buffer, sizeof(buffer), inputfile) != NULL) 
		{
			send(acceptSocket, buffer, sizeof(buffer), 0);
		}

		// Close input file
		fclose(inputfile);
	}
 	close(acceptSocket);
}

// Close sockets
close(listenSocket);
}