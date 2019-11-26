#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "commonProto.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

struct connection connections[10];
pthread_t threads[10];
pthread_t clientRevThread;
pthread_t timerThread;
int dashROption = 0;
int hasAcceptedAtLeastOneClient = 0;
int numReady;
fd_set readset;
bool receivetimeout;
bool sendtimeout;
clock_t time1;
clock_t time2;
void setClientTimeOut(int socket, int timeout){
    fprintf(stderr,"socket:%d\n",socket);
    struct timeval tv;
    tv.tv_sec       = 10;
    fprintf(stderr,"tv.tvsec: %d\n", tv.tv_sec);
    int ret = setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
    if(ret == -1){
        printf("shm_open error, errno(%d): %s\n", errno, strerror(errno));
    }
    fprintf(stderr,"timeout set:%d\n", ret);
}


void* clientThreadHandler(void* socket){
    while(1){
        char buffer[1024];
        int fd = *(int *) socket;
        int numbytes = -1;
        time1 = clock();
        if ((numbytes = recv(fd, buffer, (sizeof buffer) - 1, 0)) == -1) {
            perror("recv");
            break;
        }
        fprintf(stderr, "%s", "TEST2\n");
        if (numbytes == 0) {
                break;
        }
        buffer[numbytes] = '\0';
        fprintf(stderr, "%s", buffer);
    }
    fprintf(stderr, "%s", "canceling thread for client...\n");
    pthread_cancel(clientRevThread);
    exit(0);
}

void* timer(void* timeLimit){

    while(1){
        double timedif1 = ( ((double) clock()) / CLOCKS_PER_SEC) - (((double) time1) / CLOCKS_PER_SEC);
        double timedif2 = ( ((double) clock()) / CLOCKS_PER_SEC) - (((double) time2) / CLOCKS_PER_SEC);
        if(timedif1 > (*(int*)timeLimit) && timedif2 > (*(int*)timeLimit)){
            fprintf(stderr,"timeout\n");
            break;
        }
    }
    fprintf(stderr,"exiting because of timeout\n");
    close(0);
}


void actAsClient(struct commandOptions cmdOps){
        int socketClient;
        socketClient = socket(AF_INET, SOCK_STREAM, 0);
        fprintf(stderr,"socketClient:%d\n",socketClient);
        struct sockaddr_in server_addr, client_addr;
        if (socketClient == -1) {
            fprintf(stderr, "%s", "socket creation failed...\n");
        }else{
            fprintf(stderr, "%s", "Socket successfully created..\n");
        }
        memset(&server_addr, 0, sizeof server_addr);
        memset(&client_addr,0, sizeof client_addr);
        server_addr.sin_family = AF_INET;
        if(cmdOps.hostname){
            struct hostent * hostnm = gethostbyname(cmdOps.hostname);
            // inet_ntoa(*(long*)host->h_addr_list[0]));
            server_addr.sin_addr.s_addr =  *(long*)hostnm->h_addr_list[0];
        }
        if(cmdOps.port!= 0){
            server_addr.sin_port = htons(cmdOps.port);
        }

        if(cmdOps.option_p){
            client_addr.sin_family = AF_INET;
            client_addr.sin_addr.s_addr = INADDR_ANY;
            client_addr.sin_port = htons(cmdOps.source_port);
            if(bind(socketClient,(struct sockaddr *) &client_addr, sizeof(client_addr)) == -1){
                fprintf(stderr,"bound unsuccesfully\n");
            }else{
                fprintf(stderr,"bound sucesfully\n");
            }
        }

//    setClientTimeOut(socketClient, cmdOps.timeout);
    if(connect(socketClient, (struct sockaddr * ) &server_addr, sizeof(server_addr)) == -1){
            fprintf(stderr, "%s", "connection failed...\n");
    }else{
//            if(cmdOps.timeout != 0){
    //                setClientTimeOut(socketClient, cmdOps.timeout);
//            }
            fprintf(stderr, "%s","connection estalished ..\n");
    }
        if(pthread_create(&clientRevThread, NULL, clientThreadHandler, &socketClient) < 0){
            fprintf(stderr,"%s","output thread creation error");
        } else {
            fprintf(stderr,"%s","output thread created succesfully");
        }

        if(cmdOps.timeout != 0) {
            if (pthread_create(&timerThread, NULL, timer,&cmdOps.timeout) < 0) {
                fprintf(stderr, "timer thread failed\n");
            } else {
                fprintf(stderr, "timer thread created succesfully");
            }
        }

        while(1) {
            time2 = clock();
            char line[1024];
            memset(&line,0, sizeof line);
            int len = sizeof line;
            char c;
            while(fgets(line, 1024, stdin) != NULL){
                if (send(socketClient, line, len, 0) == -1) {
                    fprintf(stderr,"send failure\n");
                } else {
                    fprintf(stderr,"send succes\n");
                }

                int linelength = strlen(line);
                if(linelength > 0 && line[linelength-1] == '\n'){
                    break;
                }

                memset(&line,0, sizeof line);
            }
//            fprintf(stderr,"out of while loop\n");
//                while (fgets(buf, sizeof(buf), stdin) != NULL) {
//                    int length = strlen(buf);
//                    while (length > 0) {
//                        int i = send(socketClient, buf, length, 0);
//                        length -= i;
//                        buf[i] = '\0';
//                        printf("client: sent %s", buf);
//                    }
//                }
        }
        fprintf(stderr,"out of loop\n");
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
             fprintf(stderr, "gotinput\n");
//            fprintf(stderr, "dashRoption:%d\n", dashROption);
//            fprintf(stderr, "hasAcceptedAtleast oen connection:%d\n", hasAcceptedAtLeastOneClient);
            for (int i = 0; i < 10; i++) {
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
           fprintf(stderr,"client disconnection\n");
            break;
        } else if(n == -1){
            fprintf(stderr,"error\n");
            continue;
        }
        fprintf(stderr,"n:%d\n",n);
        int len = strlen(buff);
        int sent = 0;
        if(n > 0){
            fprintf(stderr,buff);
            for (int i = 0; i < 10; i++) {
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
    for(int i = 0 ; i < 10 ; i++){
        if(connections[i].socketFD == fd){
//            fprintf(stderr, "killing thread:%d\n", i);
            connections[i].inUse = FALSE;
            pthread_cancel(threads[i]);
            break;
        }
    }
}


void actAsServer(unsigned int port, struct commandOptions cmdOps){
    int socketServer;
    int numConnections = 0;
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

        if(cmdOps.option_k == 1 && cmdOps.option_r == 1){
            fprintf(stderr,"enabling 10 connections");
            numConnections = 10;
        }else{
            fprintf(stderr,"enabling one connection");
            numConnections = 1;
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
            for(int i = 0; i < numConnections ; i++){
                fprintf(stderr,"i:%d",i);
                if(connections[i].inUse == FALSE){
                    fprintf(stderr,"found slot\n");
                    if(pthread_create(&threads[i], NULL, threadHandler, &connectionSocket) < 0){
                        fprintf(stderr,"%s","thread creation error\n");
                    } else {
                        fprintf(stderr,"connection: %d\n", i);
                        connections[i].inUse = TRUE;
                        fprintf(stderr,"connection %d inUse %d\n", i, connections[i].inUse);
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
          actAsServer(cmdOps.port, cmdOps);
      }else{
          fprintf(stderr,"port not given\n");
          actAsServer(0, cmdOps);
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


