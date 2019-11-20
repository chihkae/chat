#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "commonProto.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

struct connection connections[10];
pthread_t threads[10];

void* threadHandler(void* socket){
    int fd = *((int *) socket);
    int n;
    char buff[1024];
    while(1){
        n = recv(fd, buff, sizeof buff, 0);
        int sent = 0;
        if(n > 0){
            while(sent < n) {
                int sentNow;
                for (int i = 0; i < 10; i++) {
                    if (connections[i].inUse == TRUE && connections[i].socketFD != fd) {
                        if (sentNow = send(connections[i].socketFD, buff + sent, n - sent, 0) != -1) {
                            fprintf(stderr,"%s","succsfully sent message to others");
                        } else {
                            fprintf(stderr,"%s","couldn't send message to others");
                            break;
                        }
                    }
                }
                if(sentNow == -1){
                    break;
                } else {
                    sent += sentNow;
                }
            }
        }else if(n == 0 | n == -1){
            break;
        }
    }
    for(int i = 0 ; i < 10 ; i++){
        if(connections[i].socketFD == fd){
            connections[i].inUse = FALSE;
            pthread_cancel(threads[i]);
            break;
        }
    }
}

int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  fprintf(stderr,"%s","dfdfdfdfd");
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  fprintf(stderr,"%s", "dfdfdfdf");
  int socketServer;
  struct sockaddr_storage their_addr;
  struct sockaddr_in serverAddr;
  //creating the socket
  socketServer = socket(AF_INET, SOCK_STREAM, 0);
  //setting properties of teh serverAddr
  memset(&serverAddr, 0, sizeof serverAddr);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddr.sin_port = htons(SERVER_PORT);


    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){
        fprintf(stderr,"%s", "succesfully binded");
        //20 maximum connection requests
            int connectionNumber = 0;
            pthread_t tid;
            //list of connections
            listen(socketServer,20);
            while(1){
                struct addrinfo clientAddress;
                int connectionSocket;
                connectionSocket = accept(socketServer,(struct sockaddr *)&their_addr, (int*)sizeof clientAddress);
                if(connectionSocket == -1) {
                    fprintf(stderr,"%s", "connection denied");
                    continue;
                }
                fprintf(stderr,"%s","connection accepted");
                for(int i = 0; i < 10 ; i++){
                    if(connections[i].inUse == FALSE){
                        connections[i].inUse = TRUE;
                        connections[i].socketFD = connectionSocket;
                        if(pthread_create(&threads[i], NULL, threadHandler, &connectionSocket) < 0){
                            fprintf(stderr,"%s","thread creation error");
                        } else {
                            fprintf(stderr,"%s","thread created succesfully");
                        }
                        break;
                    }
                }
            }
    }else{
        fprintf(stderr,"%s","server: bind failure");
        close(socketServer);
    }

  
  printf("Command parse outcome %d\n", retVal);

  printf("-k = %d\n", cmdOps.option_k);
  printf("-l = %d\n", cmdOps.option_l);
  printf("-v = %d\n", cmdOps.option_v);
  printf("-r = %d\n", cmdOps.option_r);
  printf("-p = %d\n", cmdOps.option_p);
  printf("-p port = %d\n", cmdOps.source_port);
  printf("Timeout value = %d\n", cmdOps.timeout);
  printf("Host to connect to = %s\n", cmdOps.hostname);
  printf("Port to connect to = %d\n", cmdOps.port);  
}
