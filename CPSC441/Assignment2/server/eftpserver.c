#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

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
struct RQ formatRQ(int sessionNumber, char filename[255]){
    struct RQ rq;
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
    token = strtok(NULL, " ");
    char *password = token;

    // Socket Creation
    int mySocket;
    mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(mySocket == -1){
        printf("Could not setup a socket\n");
        }
    
    // Client Socket
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Binding
    int status;
    struct sockaddr_in ip_server;
    struct sockaddr *server;
    memset ((char*) &ip_server, 0, sizeof(ip_server));
    ip_server.sin_family = AF_INET;
    ip_server.sin_port = htons(atoi(argv[2]));
    ip_server.sin_addr.s_addr = htonl(INADDR_ANY);
    server = (struct sockaddr *) &ip_server;
    status = bind(mySocket, server, sizeof(ip_server));
    if(status == -1){
        printf("Could not bind to port\n");
        return -1;
    }
    while(1==1){  
        printf("Waiting for message from client\n");

        // -- Authentication Phase --
        // Recieve AUTH
        struct AUTH * recvAUTH = malloc(sizeof(struct AUTH));
        readbytes = recvfrom(mySocket, recvAUTH, sizeof(*recvAUTH), 0, 
        (struct sockaddr *)&client_address, &client_address_len);
        if ( readbytes<0){
            printf("Read error\n");
            return -1;
            }

        if((strcmp(recvAUTH->username, username) != 0) || (strcmp(recvAUTH->password, password) != 0)){
            // If incorrect username or incorrect password
            // Send error message
            char errorMessage[512] = "Incorrect username or password. Terminating transmission";
            struct ERROR error;
            error = formatERROR(errorMessage);

            int readbytes = sendto(mySocket, (struct ERROR*)&error, (1024+sizeof(error)), 0, (struct sockaddr *)&client_address,
            sizeof(client_address));
            if(readbytes == -1) printf("Unsuccessful send\n");
            printf("shouldnt be here");
        }
        srand(time(NULL)); // Initialize the random number generator with the current time
        sessionNumber = (rand() % 65535) + 1;   

        // Send ACK
        struct ACK sentACK; 
        sentACK = formatACK(sessionNumber, 0, 0);
        readbytes = sendto(mySocket, (struct ACK*)&sentACK, (1024+sizeof(sentACK)), 0, (struct sockaddr *)&client_address,
        sizeof(client_address));
        if(readbytes == -1) printf("Unsuccessful send\n");

        // Socket Creation for new socket
        int newSocket;
        newSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(newSocket == -1){
            printf("Could not setup a socket\n");
        }

        // Binding
        memset ((char*) &ip_server, 0, sizeof(ip_server));
        ip_server.sin_family = AF_INET;
        ip_server.sin_port = htons(sessionNumber);
        ip_server.sin_addr.s_addr = htonl(INADDR_ANY);
        server = (struct sockaddr *) &ip_server;
        status = bind(newSocket, server, sizeof(ip_server));
        if(status == -1){
            printf("port is %d", sessionNumber);
            printf("Could not bind to port later\n");
            return -1;
    }
    

        // -- Request Phase --
        // Recieve read/write
        // Recieve RQ
        struct RQ * recvRQ = malloc(sizeof(struct RQ));
        readbytes = recvfrom(newSocket, recvRQ, sizeof(*recvRQ), 0, 
        (struct sockaddr *)&client_address, &client_address_len);
        if ( readbytes<0){
            printf("Read error\n");
            return -1;
        }
        char filename[512];
        sprintf(filename,"%s/%s", argv[3],recvRQ->filename);
        // Figure out if it's a read or a write
        if(recvRQ->opcode == 2){// Read instance (downloading from server)
            FILE *inputfile;
            inputfile = fopen(filename, "r"); // rb handles both txt and binary

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
            // This is where we begin the download loop
            while(1==1){ 
                // Send 1kb or less of data
                for(sendData.segNum < 8; sendData.segNum = (sendData.segNum % 8) + 1;){
                    fbytes = fread(sendData.segData, 1, 1024, inputfile);
                    sendData.fbytes = fbytes;
                    readbytes = sendto(newSocket, (struct DATA*)&sendData, (1024+sizeof(sendData)), 
                    0, (struct sockaddr *)&client_address, client_address_len);
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
                        FD_SET(newSocket, &fds);

                        int ready = select(newSocket+1, &fds, NULL, NULL, &tv);
                        // Select fails completely for some reason
                        if (ready < 0) {
                            perror("select failed");
                            exit(EXIT_FAILURE);
                        } 
                        // Select times out (exceeds 5 second limit)
                        else if (ready == 0 && i < 3) {
                            // timeout
                            printf("Timeout. Resending DATA...\n");
                            readbytes = sendto(newSocket, (struct DATA*)&sendData, (1024+sizeof(sendData)), 
                            0, (struct sockaddr *)&client_address, client_address_len);
                            if(readbytes == -1) printf("Unsuccessful send\n");
                            continue; // restart the loop
                        } 
                        // Retrans maxed out
                        else if (i == 3){
                            printf("No ACK received from client after max retries. Exiting...\n");
                            fbytes = 0;
                        }
                        // Can recieve ACK
                        else if (FD_ISSET(newSocket, &fds)) {
                            // receive ACK packet
                            struct ACK * recvACK = malloc(sizeof(struct ACK));
                            readbytes = recvfrom(newSocket, recvACK, sizeof(*recvACK), 0, 
                            (struct sockaddr *)&client_address, &client_address_len);
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
            close(newSocket);
        }
        else if(recvRQ->opcode == 3){// Write instance (upload)

            // Send ACK 
            sentACK = formatACK(sessionNumber, 1, 0);
            readbytes = sendto(newSocket, (struct ACK*)&sentACK, (1024+sizeof(sentACK)), 0, (struct sockaddr *)&client_address,
            sizeof(client_address));
            if(readbytes == -1) printf("Unsuccessful send\n");

            if (access(filename, F_OK) != -1) 
            {
                printf("Output file already exists. Terminating.\n");
                continue;
            }

            // Prepare download file
            FILE *outputfile;
            outputfile = fopen(filename, "wb"); // wb handles both txt and binary
            int fbytes;
            while(1 == 1){
                // Recieve data
                struct DATA * recvDATA = malloc(sizeof(struct DATA));
                readbytes = recvfrom(newSocket, recvDATA, sizeof(*recvDATA), 0, 
                (struct sockaddr *)&client_address, &client_address_len);
                if (readbytes<0){
                    printf("Read error\n");
                    return -1;
                }
                // Write data to file
                fbytes = fwrite(recvDATA->segData, recvDATA->fbytes,1,outputfile);

                // Send acknowledgement
                struct ACK sentACK;
                sentACK = formatACK(sessionNumber, recvDATA->blockNum, recvDATA->segNum);
                readbytes = sendto(newSocket, (struct ACK*)&sentACK, (1024+sizeof(sentACK)), 0, (struct sockaddr *)&client_address,
                client_address_len);
                if(readbytes == -1) printf("Unsuccessful send\n");
                if(recvDATA->fbytes < 1024) break; // This implies we have no more text to recieve and should end
                memset(recvDATA->segData, 0 , 1024);
            }
        fclose(outputfile);
        close(newSocket);
        }
    }
    close(mySocket);
    return 0;
}