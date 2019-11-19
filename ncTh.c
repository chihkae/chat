#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "commonProto.h"
#include <stdlib.h>
#include <errno.h>
#include <Thread.h>
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


struct connection* connections[10];
void* threadHandler(void* socket){
    int fd = *((int *) socket);
    int n;
    char buff[256];
    while(1){
        n = recv(fd, buff, sizeof buff, 0);
        if(n > 0){
            for(int i = 0 ; i < 10 ; i++){
                if(*(connections->i->inUse) == true && *(connections->i->socketFD) != fd){
                    if(send(*(connections->i->socketFD), buff, n , 0) != -1){
                        printf("succsfully sent message to others")
                    }else{
                        printf("couldn't send message to others");
                    }
                }
            }
        }else{
            printf("nothign to send");
        }
    }
}

int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  int socketServer;
  int connectionSocket;
  struct addrinfo serverAddr;
  //creating the socket
  socketServer = socket(AF_INET, SOCK_STREAM, 0);
  //setting properties of teh serverAddr
  memset(&serverAddr, 0, sizeof serverAddr);
  serverAddr.ai_family = AF_NET;
  serverAddr.ai_socktype = SOCK_STREAM;
  serverAddr.ai_flags = AI_PASSIVE;



    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){
        //20 maximum connection requests
            int connectionNumber = 0;
            pthread_t tid;
            //list of connections
            struct connection* connections[10];
            listen(sockid,20);
            while(1){
                connectionSocket = accept(socketServer, );
                if(connectionSocket == -1) {
                    printf("accept error");
                    continue;
                }
                printf("connection accepted");
                for(int i = 0; i < connections.length ; i++){
                    if(*(connections->i->inUse) == false){
                        *(connections->i->inUse) = true;
                        *(connections->i->socketFD) = *(int *) connectionSocket;
                        if(pthread_create(&i, NULL, threadHandler, (&connectionSocket) < 0)) {
                            printf("thread creation error");
                        } else {
                            printf("thread created succesfully");
                        }
                        break;
                    }
                }
            }
    }else{
      close(sockid);
      printf("server: bind failure");
    }




//  int sockfd;
//  if(retVal == PARSE_OK) {
//    for()
//       sockfd = socket(AF_INET, SOCK_STREAM, 0);
//    if (sockfd == -1) {
//        printf("socket creation failed...\n");
//        exit(0);
//    }
//    else
//        memset()
//  }
//
//  int sockfd, numbytes;
//  char buf[MAXDATASIZE];
//  struct addrinfo hints, *servinfo, *p;
//  int rv;
//
//  memset(&hints, 0, sizeof hints);
//  hints.ai_family = AF_UNSPEC;
//  hints.ai_socktype = SOCK_STREAM;
//
//  if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
//    // fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
//    return 1;
//  }

  
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
