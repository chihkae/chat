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

//for server only array of 10 connection structs with each elem containing fd, and inUse boolean
struct connection connections[10];
int currentConnections = 0;
pthread_t threads[10]; //array of threads for each connection in server
pthread_t clientRevThread; //thread in client to receive message from server it's connected to
pthread_t timerThread; //thread to keep track of timeout
pthread_t connectionCloser; //thread to close server in -l without -k
int dashROption = 0; //if dashRoption is enabled
int dashKOption = 0; //if server should exit when number of connections goes to 0
int hasAcceptedAtLeastOneClient = 0;
clock_t time1; //tracks how long we have not received message from server
clock_t time2; //tracks how long client has not entered in stdin
int timelimit;

int sendall(int s, char *buf, int *len);
void* clientThreadHandler(void* socket){
    while(1){
        char buffer[1024];
        memset(&buffer[0],0, sizeof(buffer));
        int fd = *(int *) socket;
        int numbytes;
        time1 = clock();
        numbytes = recv(fd, buffer, sizeof(buffer), 0);
        if (numbytes == -1) {
            perror("recv");
            break;
        }
        if (numbytes == 0) {
            continue;
        }
        if(numbytes > 0) {
            int len = strlen(buffer);
            if (write(1, buffer, len) < 0) {
                fprintf(stderr, "write to stdout error\n");
            }
        }
    }
    int fd = *(int*)socket;
    fprintf(stderr, "%s", "canceling thread for client...\n");
    if(close(fd) < 0){
        fprintf(stderr,"socket close failure\n");
    }
    pthread_cancel(clientRevThread);
    exit(0);
}

//function that runs inside the timeout thread
//if the time difference between when client last received message from server or the time difference between when
//client entered message to stdin is greater than the time limit threshold specified by the user the client is to
//disconnect
void* timer(void* socket){
    int fd = *(int*)socket;
    //keep looping to see if the client has timed out
    while(1){
        double timedif1 = ( ((double) clock()) / CLOCKS_PER_SEC) - (((double) time1) / CLOCKS_PER_SEC);
        double timedif2 = ( ((double) clock()) / CLOCKS_PER_SEC) - (((double) time2) / CLOCKS_PER_SEC);
        if(timedif1 > timelimit && timedif2 > timelimit){
            fprintf(stderr,"timeout\n");
            break;
        }
    }
    fprintf(stderr,"exiting because of timeout\n");
    if(close(fd) < 0){
        fprintf(stderr,"socket closing failure\n");
    }
    exit(0);
}


void actAsClient(struct commandOptions cmdOps){
        int socketClient;
        socketClient = socket(AF_INET, SOCK_STREAM, 0);
        fprintf(stderr,"socketClient:%d\n",socketClient);
        struct sockaddr_in server_addr, client_addr;
        if (socketClient == -1) {
            if(cmdOps.option_v) {
                fprintf(stderr, "%s", "socket creation failed...\n");
            }
        }else{
            if(cmdOps.option_v) {
                fprintf(stderr, "%s", "Socket successfully created..\n");
            }
        }

        memset(&server_addr, 0, sizeof server_addr);
        memset(&client_addr,0, sizeof client_addr);
        server_addr.sin_family = AF_INET;
        if(cmdOps.hostname != NULL){
            struct hostent * hostnm = gethostbyname(cmdOps.hostname);
            server_addr.sin_addr.s_addr =  *(long*)hostnm->h_addr_list[0];
        }
        if(cmdOps.port!= 0){
            if(cmdOps.port < 1023 || cmdOps.port > 65535 ){
                fprintf(stderr,"invalid: using reserved ports\n");
                exit(1);
            }
            server_addr.sin_port = htons(cmdOps.port);
        }

        if(cmdOps.option_p != 0){
            if(cmdOps.source_port < 1023 || cmdOps.source_port > 65535){
                if(cmdOps.option_v) {
                    fprintf(stderr, "invalid source port: reserved ports\n");
                }
                exit(0);
            }
            client_addr.sin_family = AF_INET;
            client_addr.sin_addr.s_addr = INADDR_ANY;
            client_addr.sin_port = htons(cmdOps.source_port);
            if(bind(socketClient,(struct sockaddr *) &client_addr, sizeof(client_addr)) == -1){
                if(cmdOps.option_v) {
                    fprintf(stderr, "bound unsuccesfully\n");
                }
            }else{
                fprintf(stderr,"bound sucesfully\n");
            }
        }

        if(connect(socketClient, (struct sockaddr * ) &server_addr, sizeof(server_addr)) == -1) {
            if (cmdOps.option_v) {
            fprintf(stderr, "%s", "connection failed...\n");
            }
        }else{
            fprintf(stderr, "%s","connection estalished ..\n");
        }

        if(pthread_create(&clientRevThread, NULL, clientThreadHandler, &socketClient) < 0){
            fprintf(stderr,"%s","output thread creation error");
        } else {
            fprintf(stderr,"%s","output thread created succesfully");
        }

        if(cmdOps.timeout != 0) {
            if (pthread_create(&timerThread, NULL, timer,&socketClient) < 0) {
                fprintf(stderr, "timer thread failed\n");
            } else {
                timelimit = cmdOps.timeout;
                fprintf(stderr, "timer thread created succesfully");
            }
        }

//        while(1) {
            time2 = clock();
            fprintf(stderr,"ran timer 1\n");
            char ch[1024];
            memset(&ch,0, sizeof ch);
            int n;
            while((n =read(0, &ch, sizeof(ch))) > 0)
            {
                time2 = clock();
                fprintf(stderr,"ran timer 2\n");
                fprintf(stderr,"read:%d\n",n);
                int len = strlen(ch);
                if (sendall(socketClient, ch, &len) == -1) {
                    if(cmdOps.option_v) {
                        fprintf(stderr, "send failure\n");
                    }
                    fprintf(stderr,"send failure\n");
                    exit(0);
                } else {
                    fprintf(stderr,"send succes\n");
                }
                memset(&ch,0, sizeof ch);
                if(n < 1024){
                    fprintf(stderr,"end of filessssssssssssssssss\n");
                    continue;
                }
            }
//        }
}

//helper to avoid partial sends, make sure we send all the input
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

//helper for server to check if it can close it's socket upon disconnection from all clients in -r
int checkIfConnectionCountIsEmpty(){
    for(int i =0 ; i < 10; i++){
        if(connections[i].inUse == TRUE){
            return 0;
        }
    }
    return 1;
}

//used by server to read from stdin and broadcast to all clients connected
void* reader(void* socket){
    //server socket fd
    int fd = *((int*)socket);
    char ch[1024];
    int len = sizeof ch;
    //keep reading on stdin
    while(1) {
        memset(&ch, 0, sizeof ch);

        //read from stdin and send to all other clients connected to server
        while(read(0, &ch, sizeof(ch)) > 0)
        {
            int len = strlen(ch);
            for (int i = 0; i < 10; i++) {
                if (connections[i].socketFD != fd && connections[i].inUse == TRUE) {
                    if (sendall(connections[i].socketFD,ch, &len) == -1) {
//                        fprintf(stderr, "sendall failed\n");
                    } else {
//                        fprintf(stderr, "sendall success\n");
                    }
                }
            }
            if(ch[len-1] == EOF){
                fprintf(stderr,"end of file\n");
                break;
            }
            memset(&ch, 0, sizeof ch);
        }
    }
    exit(0);
}
//used for -k without -r to ckeck if the server should close when all clients exit
void * closeServerUponAllConectionsLeft(void* socketServer){
    while(1) {
        int allconnectionsClosed = 1;
        for (int i = 0; i < 10; i++) {
            if(connections[i].inUse == TRUE){
                allconnectionsClosed = 0;
            }
        }
        if(allconnectionsClosed == 1 && hasAcceptedAtLeastOneClient == 1){
            fprintf(stderr,"all connections closed\n");
            break;
        }
    }
    close(*(int*)socketServer);
    exit(0);

}


//used to read messages from the client and send to all other connections
void* threadHandler(void* socket){
    int fd = *((int *) socket);
    int n;
    char buff[1024];
    //keep looping to see if the client sent message
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

        int len = strlen(buff);
        //server received input from the client
        if(n > 0){
            if(write(1,buff,len) < 0){
                fprintf(stderr,"write to stdout error\n");
            }

            //send message to all other clients connected to the server
            for (int i = 0; i < 10; i++) {
                //not send the same message back to the client that sent the message
                if (connections[i].inUse == TRUE && connections[i].socketFD != fd) {
                    if (sendall(connections[i].socketFD, buff, &len) == -1) {
                        fprintf(stderr,"sendall failed\n");
                    } else {
                        fprintf(stderr,"sendall success\n");
                    }
                }
            }
            memset(&buff[0],0, sizeof(buff));
        }
    }
    //given that a client with fd has disconnected, make slot in connections array previously occupied to be free
    for(int i = 0 ; i < 10 ; i++){
        if(connections[i].socketFD == fd){
            connections[i].inUse = FALSE;
            currentConnections--;
            //kill thread since it's no longer connected to the server
            pthread_cancel(threads[i]);
            break;
        }
    }
}


void actAsServer(struct commandOptions cmdOps){
    int socketServer; //socket with which server will receive conncetions
    int numConnections = 0; //connections server is to accept depending on -r or non
    pthread_t pthreadServerSend; //thread to read from stdin and send to all other conections
    struct sockaddr_in serverAddr; //for setting addrinfo of server
    socklen_t serverAsClientaddr_size = sizeof serverAddr;

    //creating the socket of the server
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    //setting properties of teh serverAddr
    memset(&serverAddr, 0, sizeof serverAddr);
    memset(&connections, 0, sizeof connections);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serverAddr.sin_zero), 8);

    if(cmdOps.port != 0) {
        fprintf(stderr,"port given: %d",cmdOps.port);
        if(cmdOps.port < 1023 || cmdOps.port > 65535){
            fprintf(stderr,"invalid input reserved ports\n");
            exit(1);
        }
        serverAddr.sin_port = htons(cmdOps.port);
    }


    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){
        //creating thread to broadcast from server to all other connection
        if(pthread_create(&pthreadServerSend, NULL, reader, &socketServer) < 0){
            fprintf(stderr,"%s","server sender thread creation error\n");
        } else {
            fprintf(stderr,"%s","server sender thread created succesfully\n");
        }

        if(cmdOps.option_r == 1){
            numConnections = 10;
        }else{
            numConnections = 1;
        }
        fprintf(stderr,"number of connections:%d",numConnections);

        if (listen(socketServer, 20) == -1) {
            if(cmdOps.option_v){
                fprintf(stderr,"listen failure\n");
            }
            if(close(socketServer) < 0){
                fprintf(stderr,"socket closing error\n");
            }
            exit(1);
        }

        //create a thread for closing the progrma when there are no more clients connected given that -k is not provided
        if(cmdOps.option_k == 0){
            if(pthread_create(&connectionCloser,NULL, closeServerUponAllConectionsLeft, &socketServer) < 0){
                fprintf(stderr,"all connections closed thread creation error\n");
            }
        }


        //keep looping to accept more incoming connections
        while(1){

            struct addrinfo clientAddress;
            //fd of the client, to be overwritten for every new client
            int connectionFD;
            socklen_t addr_size = sizeof clientAddress;
            //loop over connections array to find if we have a slot to accept the incoming connection
            for(int i = 0; i < numConnections ; i++){
                if(connections[i].inUse == FALSE){
                    fprintf(stderr,"waiting for a connection\n");
                    //accept the connection and set the fd of the client
                    connectionFD = accept(socketServer,(struct sockaddr *)&clientAddress, &addr_size);
                    if(connectionFD == -1) {
                        if (cmdOps.option_v) {
                            fprintf(stderr, "%s", "connection denied\n");
                        }
                        continue;
                    }
                    fprintf(stderr,"accpeted a connection\n");
                    //create thread for each incoming connection
                    if(pthread_create(&threads[i], NULL, threadHandler, &connectionFD) < 0){
                        fprintf(stderr,"%s","thread creation error\n");
                    } else {
                        fprintf(stderr,"connection: %d\n", i);
                        connections[i].inUse = TRUE;
                        fprintf(stderr,"connection %d inUse %d\n", i, connections[i].inUse);
                        connections[i].socketFD = connectionFD;
                        hasAcceptedAtLeastOneClient = 1;
                        currentConnections++;
                        //a slot was found for the connection and inserted into array so no need to check for more slots
                        break;
                    }
                }else{
                    continue;
                }
            }
        }
    } else {
        if(cmdOps.option_v){
            fprintf(stderr,"socket binding failure\n");
        }
        if(close(socketServer) < 0){
            fprintf(stderr,"socket closing failure\n");
        }
        exit(0);
    }
    close(socketServer);
}


int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  //for debugging purposes let user know what options they have specified that  are properly parsed
  if(cmdOps.option_v == 0) {
      fprintf(stderr, "option-k:%d\n", cmdOps.option_k);
      fprintf(stderr, "option-l:%d\n", cmdOps.option_l);
      fprintf(stderr, "option-v:%d\n", cmdOps.option_v);
      fprintf(stderr, "option-r:%d\n", cmdOps.option_r);
      fprintf(stderr, "option-p:%d\n", cmdOps.option_p);
      fprintf(stderr, "option-source port:%d\n", cmdOps.source_port);
      fprintf(stderr, "co-timeout:%d\n", cmdOps.timeout);
      fprintf(stderr, "option-hostname:%s\n", cmdOps.hostname);
      fprintf(stderr, "option-port:%d\n", cmdOps.port);
  }
  //run program only if parsing was done properly and no options contradict one another
  if(retVal != PARSE_OK){
        fprintf(stderr,"parsing error\n");
        exit(0);
  }
  dashROption = cmdOps.option_r;
  dashKOption = cmdOps.option_k;
  //option l means to run as server or else run as client
  if(cmdOps.option_l == 1){
      if(cmdOps.option_v){
          fprintf(stderr,"acting as server\n");
      }
      actAsServer(cmdOps);
      exit(0);
  }else {
      if(cmdOps.option_v){
          fprintf(stderr,"acting as client\n");
      }
      actAsClient(cmdOps);
      exit(0);
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
  exit(0);
}


