#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
int main()
  {
 struct sockaddr_in address;
 memset(&address, 0, sizeof(address));
 address.sin_family = AF_INET;
 address.sin_port = htons(2000);
 address.sin_addr.s_addr = htonl(INADDR_ANY);
 int mysocket1;
 mysocket1 = socket(AF_INET, SOCK_STREAM, 0);
 if(mysocket1 == -1){
 	printf("socket() call failed");
 }
 
 int status;
 status = bind(mysocket1, (struct sockaddr *)
 &address, sizeof(struct sockaddr_in));
 if(status==-1){
 	printf("bind() call failed");
}
 status = listen(mysocket1,5);
 if(status==-1){
 	printf("listen() call failed");
 }
  int mysocket2;
 mysocket2 = accept(mysocket1, NULL, NULL);
 if(mysocket2 == -1){
 	printf("accept() call failed");
 }
 int count;
 char rcv_message[100];
 count = recv(mysocket2, rcv_message, 100, 0);
 if(count == -1){
 	printf("recv() call failed.");
 }
 printf("%s", rcv_message);

 char snd_message[100] = {"test"};
 count = send(mysocket1, snd_message, 4, 0);


 close(mysocket2);
 close(mysocket1);
  }
