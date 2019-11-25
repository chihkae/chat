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
#include <sys/time.h>

struct connection connections[11];
pthread_t threads[10];
pthread_t clientRevThread;
int dashROption = 0;
int hasAcceptedAtLeastOneClient = 0;

void setClientTimeOut(int socket, int timeout){
    struct timeval tv;
    tv.tv_sec = 5;
    setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&tv, sizeof(struct timeval));
    fprintf(stderr,"timeout set\n");
}


void* clientThreadHandler(void* socket){
    while(1){
        char buffer[1024];
        int * fd = (int *) socket;
        int numbytes = -1;
        if ((numbytes = recv(*fd, buffer, (sizeof buffer) -1, 0)) == -1) {
            perror("recv");
            break;
        }
        fprintf(stderr, "%s", "TEST2\n");
        if(numbytes == 0){
            break;
        }
        buffer[numbytes] = '\0';
        fprintf(stderr, "%s", buffer);

    }
    fprintf(stderr, "%s", "canceling thread for client...\n");
    pthread_cancel(clientRevThread);
}

void readClientInput(){
    //
}


void actAsClient(struct commandOptions cmdOps){
        int socketClient;
        socketClient = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in cli_addr, cli_addr1;
        if (socketClient == -1) {
            fprintf(stderr, "%s", "socket creation failed...\n");
        }else{
            fprintf(stderr, "%s", "Socket successfully created..\n");
        }
        memset(&cli_addr, 0, sizeof cli_addr);
        cli_addr.sin_family = AF_INET;
        if(cmdOps.hostname){
            struct hostent * hostnm = gethostbyname(cmdOps.hostname);
            // inet_ntoa(*(long*)host->h_addr_list[0]));
            cli_addr.sin_addr.s_addr =  *(long*)hostnm->h_addr_list[0];
        }
        cli_addr.sin_port = htons(cmdOps.port);

        if(cmdOps.option_p){
            memset(&cli_addr1, 0, sizeof cli_addr1);
            cli_addr1.sin_family = AF_INET;
            cli_addr1.sin_port = htons(cmdOps.source_port);
        }
        if(cmdOps.timeout != 0){
            setClientTimeOut(socketClient, cmdOps.timeout);
        }
        if (bind(socketClient, (struct sockaddr*) &cli_addr1, sizeof(struct sockaddr_in)) == 0) {
            fprintf(stderr, "%s", "Binded Correctly\n");
        }else {
            fprintf(stderr, "%s", "Unable to bind\n");
        }

        if(connect(socketClient, (struct sockaddr * ) &cli_addr, sizeof(cli_addr)) == -1){
            fprintf(stderr, "%s", "connection failed...\n");
        }else
        {
            fprintf(stderr, "%s","connection estalished ..\n");
        }


        if(pthread_create(&clientRevThread, NULL, clientThreadHandler, &socketClient) < 0){
            fprintf(stderr,"%s","output thread creation error");
        } else {
            fprintf(stderr,"%s","output thread created succesfully");
        }
        char buf[100];
        memset(&buf[0],0, sizeof(buf));
        fprintf(stderr, "%s","Enter a message: \n");

        while(fgets(buf, sizeof(buf) , stdin) != NULL ){
            int length =  strlen(buf);
            while (length > 0)
            {
                int i = send(socketClient, buf, length, 0);
                length -= i;
                buf[i] = '\0';
                printf("client: sent %s", buf);
            }
        }

}

int sendall(int s, char *buf, int *len){
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
        n = write(s, buf+total, bytesleft);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here
    return n==-1?-1:0; // return -1 on failure, 0 on success
}
int checkIfConnectionCountIsEmpty(){
    for(int i =1 ; i < 11; i++){
        if(connections[i].inUse == TRUE){
            return 0;
        }
    }
    return 1;
}


void* reader(void* socket){
    int fd = *((int*)socket);
    connections[0].inUse = TRUE;
    connections[0].socketFD = fd;
//    fprintf(stderr,"connection 0:%d inside reader\n", connections[0].socketFD);
    char line[1024];
    int len = sizeof line;
    memset(&line, 0, sizeof line);

    while(1) {
        if (dashROption == 1 && hasAcceptedAtLeastOneClient == 1) {
            fprintf(stderr, "checking");
            if (checkIfConnectionCountIsEmpty() == 1) {
                fprintf(stderr, "No more clients,all connections closed\n");
                close(fd);
                exit(0);
            }
        }
        while (fgets(line, 100, stdin) != NULL) {

//            fprintf(stderr, "gotinput\n");
//            fprintf(stderr, "dashRoption:%d\n", dashROption);
//            fprintf(stderr, "hasAcceptedAtleast oen connection:%d\n", hasAcceptedAtLeastOneClient);
            for (int i = 1; i < 11; i++) {
//                fprintf(stderr, "from stndard input");
//                fprintf(stderr, "loop %d, inUse: %d socketFD: %d\n", i, connections[i].inUse, connections[i].socketFD);
                if (connections[i].socketFD != fd && connections[i].inUse == TRUE) {
                    if (sendall(connections[i].socketFD, line, &len) == -1) {
//                        fprintf(stderr, "sendall failed\n");
                    } else {
//                        fprintf(stderr, "sendall success\n");
                    }
                }
            }
            int linelength = strlen(line);
            if(linelength > 0 && line[linelength-1] == '\n')
            {
//                fprintf(stderr,"breaking for new line");
                break;
            }
            memset(&line, 0, sizeof line);
        }
    }
}



void* threadHandler(void* socket){
    int fd = *((int *) socket);
//    fprintf(stderr,"fd:%d\n",fd);
    int n;
    char buff[1024];
//    fprintf(stderr,"connection 0 fd:%d\n",connections[0].socketFD);
    while(1){
        memset(&buff[0],0, sizeof(buff));
        n = recv(fd, buff, sizeof buff, 0);
        if(n == 0){
//            fprintf(stderr,"didn't receive jack\n");
            break;
        }
//        fprintf(stderr,"n:%d\n",n);
        int len = strlen(buff);
        int sent = 0;
        if(n > 0){
            fprintf(stderr,buff);
            for (int i = 1; i < 11; i++) {
                if (connections[i].inUse == TRUE && connections[i].socketFD != fd) {
//                    fprintf(stderr,"loop %d, inUse: %d socketFD: %d\n", i, connections[i].inUse, connections[i].socketFD);
//                    fprintf(stderr,"mehhh\n");
//                    fprintf(stderr,"checking connections\n");
                    if (send(connections[i].socketFD, buff, sizeof(buff), 0) == -1) {
//                        fprintf(stderr,"sendall failed\n");
                    } else {
                        fprintf(stderr,"sendall success\n");
                    }
                }
            }
            memset(&buff[0],0, sizeof(buff));
        }
    }
    for(int i = 1 ; i < 11 ; i++){
        if(connections[i].socketFD == fd){
//            fprintf(stderr, "killing thread:%d\n", i);
            connections[i].inUse = FALSE;
            pthread_cancel(threads[i]);
            break;
        }
    }
}


void actAsServer(unsigned int port){
    int socketServer;
    int socketServerAsClient;
    pthread_t pthreadServerSend;
    pthread_t pthreadServerRec;
    struct sockaddr_in serverAddr;
    socklen_t serverAsClientaddr_size = sizeof serverAddr;

    //creating the socket
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    //setting properties of teh serverAddr
    memset(&serverAddr, 0, sizeof serverAddr);
    memset(&connections, 0, sizeof connections);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serverAddr.sin_zero), 8);
    if(port != 0) {
        fprintf(stderr,"port given: %d",port);
        serverAddr.sin_port = htons((int)port);
    }


    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){
//        fprintf(stderr,"%s", "succesfully binded\n");
        //list of connections
        if (listen(socketServer, 20) == -1) {
//            fprintf(stderr,"listen error\n");
            exit(1);
        }

        if(pthread_create(&pthreadServerSend, NULL, reader, &socketServer) < 0){
            fprintf(stderr,"%s","server sender thread creation error\n");
        } else {
            fprintf(stderr,"%s","server sender thread created succesfully\n");
        }



        while(1){
//            fprintf(stderr,"fdsfsdf");
            struct addrinfo clientAddress;
            int connectionSocket;
            socklen_t addr_size = sizeof clientAddress;

//            fprintf(stderr,"1");
            connectionSocket = accept(socketServer,(struct sockaddr *)&clientAddress, &addr_size);
            if(connectionSocket == -1) {
                fprintf(stderr,"%s", "connection denied\n");
                continue;
            }
            fprintf(stderr,"%s","connection accepted\n");
            for(int i = 1; i < 11 ; i++){
                if(connections[i].inUse == FALSE){
                    if(pthread_create(&threads[i], NULL, threadHandler, &connectionSocket) < 0){
//                        fprintf(stderr,"%s","thread creation error\n");
                    } else {
//                       fprintf(stderr,"connection: %d\n", i);
                        connections[i].inUse = TRUE;
//                        fprintf(stderr,"connection %d inUse %d\n", i, connections[i].inUse);
                        connections[i].socketFD = connectionSocket;
//                        fprintf(stderr,"connection %d socketFD:%d\n", i,connections[i].socketFD);
//                        fprintf(stderr,"%s","thread created succesfully\n");
                        hasAcceptedAtLeastOneClient = 1;
//                        if(pthread_create(&receiverthreads[i], NULL,receiverHandler, &connectionSocket) < 0){
////            fprintf(stderr,"%s","server reeiver thread creation error\n");
//                        }else{
////            fprintf(stderr,"%s","server receiver thread created succesfully\n");
//                        }
                        break;
                    }
                }
            }
        }
    } else {
//        fprintf(stderr,"%s","server: bind failure");
        close(socketServer);
    }
}


int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  fprintf(stderr,"dash r option:%d", cmdOps.option_r);
  if(cmdOps.option_r == 1){
      dashROption = 1;
  }

  if(cmdOps.option_l == 1){
      if(cmdOps.port != (unsigned  int)0){
          fprintf(stderr,"port given\n");
          actAsServer(cmdOps.port);
      }else{
          fprintf(stderr,"port not given\n");
          actAsServer(0);
      }
  }

  actAsClient(cmdOps);

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


