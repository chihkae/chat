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


int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

void* reader(void* socket){
    fprintf(stderr,"inside broadcaset function");
    char line[100];
    int len = sizeof line;

    while ( fgets( line, 100, stdin ) != NULL )
    {
        fprintf(stderr,"gotinput");
        for (int i = 0; i < 10; i++) {
            fprintf(stderr,"from stndard input");
            fprintf(stderr, "loop %d, inUse: %d socketFD: %d\n", i, connections[i].inUse, connections[i].socketFD);
            if (connections[i].inUse == TRUE) {
                if (sendall(connections[i].socketFD, line, &len) == -1) {
                    fprintf(stderr, "sendall failed\n");
                } else {
                    fprintf(stderr, "sendall success\n");
                }
            }
        }
    }
}


void* threadHandler(void* socket){
    int fd = *((int *) socket);
    fprintf(stderr,"fd:%d\n",fd);
    int n;
    char buff[1024];
    while(1){
        memset(&buff[0],0, sizeof(buff));
        n = recv(fd, buff, sizeof buff, 0);
        if(n < 0){
            continue;
        }
        fprintf(stderr,"n:%d\n",n);
        int len = strlen(buff);
        int sent = 0;
        if(n > 0){
            for (int i = 0; i < 10; i++) {
                fprintf(stderr,"loop %d, inUse: %d socketFD: %d\n", i, connections[i].inUse, connections[i].socketFD);
                if (connections[i].inUse == TRUE && connections[i].socketFD != fd) {
                    if (sendall(connections[i].socketFD, buff, &len) == -1) {
                        fprintf(stderr,"sendall failed\n");
                    } else {
                        fprintf(stderr,"sendall success\n");
                    }
                }
            }
        }
    }
    for(int i = 0 ; i < 10 ; i++){
        if(connections[i].socketFD == fd){
            fprintf(stderr, "killing thread:%d\n", i);
            connections[i].inUse = FALSE;
            pthread_cancel(threads[i]);
            break;
        }
    }
}

void broadcastToAll(){
    fprintf(stderr,"inside broadcaset function");
    int    c;
    int len = 100;

    char line[100];
    while ( fgets( line, 100, stdin ) != NULL )
    {
        fprintf(stderr,"gotinput");
        for (int i = 0; i < 10; i++) {
            fprintf(stderr,"from stndard input");
            fprintf(stderr, "loop %d, inUse: %d socketFD: %d\n", i, connections[i].inUse, connections[i].socketFD);
            if (connections[i].inUse == TRUE) {
                if (sendall(connections[i].socketFD, line, &len) == -1) {
                    fprintf(stderr, "sendall failed\n");
                } else {
                    fprintf(stderr, "sendall success\n");
                }
            }
        }
    }
    return;
}
void actAsServer(unsigned int port){
    int socketServer;
    int socketServerAsClient;
    struct sockaddr_in serverAddr;
    socklen_t serverAsClientaddr_size = sizeof serverAddr;
    pthread_t pthread;
    //creating the socket
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    //setting properties of teh serverAddr
    memset(&serverAddr, 0, sizeof serverAddr);
    memset(&connections, 0, sizeof connections);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(port != 0) {
        fprintf(stderr,"port given: %d",port);
        serverAddr.sin_port = htons((int)port);
    }


    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){
        fprintf(stderr,"%s", "succesfully binded\n");
        //20 maximum connection requests
        int connectionNumber = 0;
        pthread_t serverThread;
        //list of connections
        listen(socketServer,20);
        if(pthread_create(&pthread, NULL, reader, &socketServer) < 0){
            fprintf(stderr,"%s","thread creation error\n");
        } else {
            fprintf(stderr,"%s","thread created succesfully\n");
        }


        while(1){
            struct addrinfo clientAddress;
            int connectionSocket;
            socklen_t addr_size = sizeof clientAddress;
            connectionSocket = accept(socketServer,(struct sockaddr *)&clientAddress, &addr_size);
            if(connectionSocket == -1) {
                fprintf(stderr,"%s", "connection denied\n");
                continue;
            }
            fprintf(stderr,"%s","connection accepted\n");
            for(int i = 0; i < 10 ; i++){
                if(connections[i].inUse == FALSE){
                    fprintf(stderr,"connection: %d\n", i);
                    connections[i].inUse = TRUE;
                    fprintf(stderr,"connection %d inUse %d\n", i, connections[i].inUse);
                    connections[i].socketFD = connectionSocket;
                    fprintf(stderr,"connection %d socketFD:%d\n", i,connections[i].socketFD);
                    if(pthread_create(&threads[i], NULL, threadHandler, &connectionSocket) < 0){
                        fprintf(stderr,"%s","thread creation error\n");
                    } else {
                        fprintf(stderr,"%s","thread created succesfully\n");
                    }
                    break;
                }
            }
        }
    }else{
        fprintf(stderr,"%s","server: bind failure");
        close(socketServer);
    }
}


int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);

  if(cmdOps.option_l == 1){
      if(cmdOps.port != (unsigned  int)0){
          fprintf(stderr,"port given\n");
          actAsServer(cmdOps.port);
      }else{
          fprintf(stderr,"port not given\n");
          actAsServer(0);
      }
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


