#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct AUTH{
    unsigned int opcode;
    char username[32];
    char password[32];
};
struct AUTH formatAUTH(char username[32], char password[32]){
    struct AUTH auth;
    auth.opcode = 01;
    strcpy(auth.username, username);
    strcpy(auth.password, password);
    return auth;
}

struct RQ{
    unsigned int opcode;
    int sessionNumber;
    char filename[255];
};
struct RQ formatRQ(unsigned int opcode, int sessionNumber, char filename[255]){
    struct RQ rq;
    rq.opcode = opcode;
    rq.sessionNumber = sessionNumber;
    strcpy(rq.filename, filename);
    return rq;
}

struct DATA{
    unsigned int opcode;
    int sessionNum;
    int blockNum;
    int segNum;
    unsigned char segData[1024];
    int fbytes;
};

struct ACK{
    unsigned int opcode;
    int sessionNum;
    int blockNum;
    int segNum;
};
struct ACK formatACK(int sessionNum, int blockNum, int segNum){
    struct ACK ack;
    ack.opcode = 05;
    ack.sessionNum = sessionNum;
    ack.blockNum = 0;
    ack.segNum = 0;
    return ack;
}

struct ERROR{
    unsigned int opcode;
    char errorMessage[512];
};
struct ERROR formatERROR(char errorMessage[512]){
    struct ERROR error;
    error.opcode = 06;
    strcpy(error.errorMessage, errorMessage);
    return error;
}

int main(int argc, char *argv[]){
    int sessionNumber;
    int readbytes;
    if(argc != 3){
        printf("invalid number of args");
        return -1;
    } 

    // Sort information as required
    char *info = argv[1];
    char *token = strtok(info, ":");
    char *username = token;
    token = strtok(NULL, "@");
    char *password = token;
    token = strtok(NULL, ":");
    char *ip = token;
    token = strtok(NULL, ":");
    char *port = token;

    // Socket Creation
    int mySocket;
    mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(mySocket == -1){
        printf("Could not setup a socket");
        }
    else printf("Socket successfully setup\n");

    // Fill in the server address information
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(port)); // Port #
    server_address.sin_addr.s_addr = inet_addr(ip);
    socklen_t server_address_len = sizeof(server_address);

    // -- Authentication Phase --

    // Format username and password
    struct AUTH sendAUTH;
    sendAUTH = formatAUTH(username, password);

    // Send AUTH
    readbytes = sendto(mySocket, (struct AUTH*)&sendAUTH, (1024+sizeof(sendAUTH)), 
    0, (struct sockaddr *)&server_address, server_address_len);
    if(readbytes == -1) printf("Unsuccessful send\n");

    // Recieve ACK
    struct ACK * recvACK = malloc(sizeof(struct ACK));
    readbytes = recvfrom(mySocket, recvACK, sizeof(*recvACK), 0, 
    (struct sockaddr *)&server_address, &server_address_len);
    if (readbytes<0){
        printf("Read error\n");
        return -1;
    }
    sessionNumber = recvACK->sessionNum;

    server_address.sin_port = htons(sessionNumber); // Port #


    // -- Request Phase --
    // Decide if it's read or write
    if(strcmp(argv[2], "download") == 0){// Read request (download)
        // Send read request
        struct RQ sentRRQ;
        sentRRQ = formatRQ(02,sessionNumber, argv[3]);
        readbytes = sendto(mySocket, (struct RQ*)&sentRRQ, (1024+sizeof(sentRRQ)), 
        0, (struct sockaddr *)&server_address, server_address_len);
        if(readbytes == -1) printf("Unsuccessful send\n");

		if (access(argv[3], F_OK) != -1) 
		{
            printf("Output file already exists. Terminating.\n");
            return 0;
		}

        // Prepare download file
        FILE *outputfile;
        outputfile = fopen(argv[3], "wb"); // wb handles both txt and binary
        int fbytes;
        while(1 == 1){
            // Recieve data
            struct DATA * recvDATA = malloc(sizeof(struct DATA));
            readbytes = recvfrom(mySocket, recvDATA, sizeof(*recvDATA), 0, 
            (struct sockaddr *)&server_address, &server_address_len);
            if (readbytes<0){
                printf("Read error\n");
                return -1;
            }
            // Write data to file
            fbytes = fwrite(recvDATA->segData, recvDATA->fbytes,1,outputfile);
            // Send acknowledgement
            struct ACK sentACK;
            sentACK = formatACK(sessionNumber, recvDATA->blockNum, recvDATA->segNum);
            readbytes = sendto(mySocket, (struct ACK*)&sentACK, (1024+sizeof(sentACK)), 0, (struct sockaddr *)&server_address,
            server_address_len);
            if(readbytes == -1) printf("Unsuccessful send\n");
            if(recvDATA->fbytes < 1024) break; // This implies we have no more text to recieve and should end
            memset(recvDATA->segData, 0 , 1024);
        }
	    fclose(outputfile);
        return 0;
    }
    else if(strcmp(argv[2], "upload") == 0){// Write request (upload)
        // Send write request
        struct RQ sentWRQ;
        sentWRQ = formatRQ(03, sessionNumber, argv[3]);
        readbytes = sendto(mySocket, (struct RQ*)&sentWRQ, (1024+sizeof(sentWRQ)), 
        0, (struct sockaddr *)&server_address, server_address_len);
        if(readbytes == -1) printf("Unsuccessful send\n");

        // Recieve ACK
        struct ACK * recvACK = malloc(sizeof(struct ACK));
        readbytes = recvfrom(mySocket, recvACK, sizeof(*recvACK), 0, 
        (struct sockaddr *)&server_address, &server_address_len);
        if (readbytes<0){
            printf("Read error\n");
            return -1;
        }
        FILE *inputfile;
            inputfile = fopen(argv[3], "r"); // rb handles both txt and binary

            if (inputfile == NULL) 
            {
                perror("Input file could not be accessed");
                return(0);
            }
        int fbytes;
        struct DATA sendData;
        sendData.opcode = 04;
        sendData.sessionNum = sessionNumber;
        sendData.blockNum = 1;
        sendData.segNum = 0;
        memset(&sendData.segData, 0, 1024);
        // This is where we begin the upload loop
        while(1==1){ 
                // Send 1kb or less of data
                for(sendData.segNum < 8; sendData.segNum = (sendData.segNum % 8) + 1;){
                    fbytes = fread(sendData.segData, 1, 1024, inputfile);
                    sendData.fbytes = fbytes;
                    readbytes = sendto(mySocket, (struct DATA*)&sendData, (1024+sizeof(sendData)), 
                    0, (struct sockaddr *)&server_address, server_address_len);
                    if(readbytes == -1) printf("Unsuccessful send\n");

                    // Wait for acknowledgement
                    // make a loop called ackloop
                    struct timeval tv;
                    // loop sending data until ACK is received (3 times max, 5 seconds between each attempt)
                    // make sure ACK is for the correct block and segment
                    for(int i = 0; i < 4; i++){
                        tv.tv_sec = 5; // 5 seconds
                        tv.tv_usec = 0;


                        fd_set fds;
                        FD_ZERO(&fds);
                        FD_SET(mySocket, &fds);

                        int ready = select(mySocket+1, &fds, NULL, NULL, &tv);
                        // Select fails completely for some reason
                        if (ready < 0) {
                            perror("select failed");
                            exit(EXIT_FAILURE);
                        } 
                        // Select times out (exceeds 5 second limit)
                        else if (ready == 0 && i < 3) {
                            // timeout
                            printf("Timeout. Resending DATA...\n");
                            readbytes = sendto(mySocket, (struct DATA*)&sendData, (1024+sizeof(sendData)), 
                            0, (struct sockaddr *)&server_address, server_address_len);
                            if(readbytes == -1) printf("Unsuccessful send\n");
                            continue; // restart the loop
                        } 
                        // Retrans maxed out
                        else if (i == 3){
                            printf("No ACK received from client after max retries. Exiting...\n");
                            fbytes = 0;
                        }
                        // Can recieve ACK
                        else if (FD_ISSET(mySocket, &fds)) {
                            // receive ACK packet
                            struct ACK * recvACK = malloc(sizeof(struct ACK));
                            readbytes = recvfrom(mySocket, recvACK, sizeof(*recvACK), 0, 
                            (struct sockaddr *)&server_address, &server_address_len);
                            if (readbytes<0){
                                printf("Read error\n");
                                return -1;
                            }
                            memset(&sendData.segData, 0, 1024);
                            break; // no more for loop if we succeeded in our goal
                        }
                    }
                    // If fbytes is less than 1024 we break both the for loop and the while loop
                    if(fbytes < 1024) break;
                }
                if(fbytes < 1024) break;
                sendData.blockNum++;
            }
            fclose(inputfile);
    }
    else{
        printf("this is not an option");
    }
    close(mySocket);
}