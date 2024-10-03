#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    // Sort information as required
    char *info = argv[1];
    char *token = strtok(info, ":");
    char *username = token;
    token = strtok(NULL, "@");
    char *password = token;
    token = strtok(NULL, ":");
    char *ip = token;
    token = strtok(NULL, " ");
    char *port = token;

    printf("%s is the username, %s is the password, %s is the ip, and %s is the port", 
    username, password, ip, port);
}