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
int currentConnections = 0; //number of clients currently connected to server
pthread_t threads[10]; //array of threads for each connection in server
pthread_t clientRevThread; //thread in client to receive message from server it's connected to
pthread_t timerThread; //thread to keep track of timeout
pthread_t connectionCloser; //thread to close server in -l without -k
int dashROption = 0; //if dashRoption is enabled
int dashKOption = 0; //if server should exit when number of connections goes to 0
int hasAcceptedAtLeastOneClient = 0; //server must have at least accepted one connection in -l before closing
clock_t time1; //tracks how long we have not received message from server
clock_t time2; //tracks how long client has not entered in stdin
int timelimit; // time limit for -w in client mode

int sendall(int s, char *buf, int *len);
//thread that runs in client mode responsible for receiving messages from the server
void* clientThreadHandler(void* socket){
    //continually check if server has message for client to receive
    while(1){
        char buffer[1024];
        memset(&buffer[0],0, sizeof(buffer));
        int fd = *(int *) socket;
        int numbytes;
        //timer is reset everytime client receives more messsage from the server
        time1 = clock();
        numbytes = recv(fd, buffer, sizeof(buffer), 0);

        if (numbytes == -1) {
            fprintf(stderr,"recv\n");
            break;
        }
        //means server disconnected, so client should close socket and exit
        if (numbytes == 0) {
            break;
        }
        if(numbytes > 0) {
            int len = strlen(buffer);
            if (write(1, buffer, len) < 0) {
                fprintf(stderr, "write to stdout error\n");
            }
        }
    }
    //if the server disconnects client should close its own socket and exit out of the program
    int fd = *(int*)socket;
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
            fprintf(stderr,"timeout, now exiting\n");
            break;
        }
    }
    if(close(fd) < 0){
        fprintf(stderr,"socket closing failure\n");
    }
    exit(0);
}

//main function for creating threads and sockets in client mode
void actAsClient(struct commandOptions cmdOps){
        int socketClient;
        socketClient = socket(AF_INET, SOCK_STREAM, 0);
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
                exit(0);
            }else{
                if(cmdOps.option_v) {
                    fprintf(stderr, "bound sucesfully\n");
                }
            }
        }

        if(connect(socketClient, (struct sockaddr * ) &server_addr, sizeof(server_addr)) == -1) {
            if (cmdOps.option_v) {
                fprintf(stderr, "%s", "connection failed...\n");
            }
            exit(0);
        }else{
            if(cmdOps.option_v){
                fprintf(stderr, "%s","connection estalished ..\n");
            }
        }

        if(pthread_create(&clientRevThread, NULL, clientThreadHandler, &socketClient) < 0){
            if(cmdOps.option_v) {
                fprintf(stderr, "%s", "output thread creation error");
            }
        } else {
            if(cmdOps.option_v) {
                fprintf(stderr, "%s", "output thread created succesfully");
            }
        }

        if(cmdOps.timeout != 0) {
            if (pthread_create(&timerThread, NULL, timer,&socketClient) < 0) {
                if(cmdOps.option_v) {
                    fprintf(stderr, "timer thread failed\n");
                }
            } else {
                timelimit = cmdOps.timeout;
                if(cmdOps.option_v) {
                    fprintf(stderr, "timer thread created succesfully");
                }
            }
        }
        //start clock for stdin
        time2 = clock();
        if(cmdOps.option_v) {
            fprintf(stderr, "ran timer 2 first time\n");
        }
        char ch[1024];
        memset(&ch,0, sizeof ch);
        int n;
        while((n =read(0, &ch, sizeof(ch))) > 0)
        {
            //stdin has been read so reset it
            time2 = clock();
            if(cmdOps.option_v) {
                fprintf(stderr, "ran timer 2 second time\n");
                fprintf(stderr, "read:%d\n", n);
            }
            int len = strlen(ch);
            if (sendall(socketClient, ch, &len) == -1) {
                if(cmdOps.option_v) {
                    fprintf(stderr, "send failure\n");
                }
            } else {
                if(cmdOps.option_v) {
                    fprintf(stderr, "send succes\n");
                }
            }
            //clear ch buffer before next read to avoid reading same info next time
            memset(&ch,0, sizeof ch);
            if(n < 1024){
                if(cmdOps.option_v) {
                    fprintf(stderr, "end of file for client\n");
                }
                continue;
            }
        }
        //means we have eof so we must exit out of the program
        if(n == 0){
            if(cmdOps.option_v) {
                fprintf(stderr, "eof\n");
            }
            exit(0);
        }
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
    int n;
    //keep reading on stdin
    while(1) {
        memset(&ch, 0, sizeof(ch));
        //read from stdin and send to all other clients connected to server
        while((n =read(0, &ch, sizeof(ch))) > 0)
        {
            int len = strlen(ch);
            for (int i = 0; i < 10; i++) {
                if (connections[i].socketFD != fd && connections[i].inUse == TRUE) {
                    if (sendall(connections[i].socketFD,ch, &len) == -1) {
                        fprintf(stderr,"send all failure\n");
                    }
                }
            }
            memset(&ch, 0, sizeof(ch));
            if(ch[len-1] == '\n'){
                fprintf(stderr,"end of file\n");
                break;
            }
        }
    }
}
//used for -k without -r to ckeck if the server should close when all clients exit
void * closeServerUponAllConectionsLeft(void* socketServer){
    sleep(3);
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
    hasAcceptedAtLeastOneClient = 1;

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

// get port, IPv4 or IPv6:
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void actAsServer(struct commandOptions cmdOps){
    int socketServer; //socket with which server will receive conncetions
    int numConnections = 0; //connections server is to accept depending on -r or non
    pthread_t pthreadServerSend; //thread to read from stdin and send to all other conections
    struct sockaddr_in serverAddr; //for setting addrinfo of server
    socklen_t serverAsClientaddr_size = sizeof serverAddr;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size; //used for getting connector's address info
    char s[INET6_ADDRSTRLEN]; //used for getting connector's address info


    //creating the socket of the server
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    //setting properties of teh serverAddr
    memset(&serverAddr, 0, sizeof serverAddr);
    memset(&connections, 0, sizeof connections);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //set the server port number if the user provided one
    if(cmdOps.port != 0) {
        if(cmdOps.option_v) {
            fprintf(stderr, "port given: %d", cmdOps.port);
        }
        if(cmdOps.port < 1023 || cmdOps.port > 65535){
            fprintf(stderr,"invalid input reserved ports\n");
            exit(1);
        }
        serverAddr.sin_port = htons((int)cmdOps.port);
    }


    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){
        //creating thread to broadcast from server to all other connection
        if(pthread_create(&pthreadServerSend, NULL, reader, &socketServer) < 0){
            if(cmdOps.option_v) {
                fprintf(stderr, "%s", "server sender thread creation error\n");
            }
        } else {
            if(cmdOps.option_v) {
                fprintf(stderr, "%s", "server sender thread created succesfully\n");
            }
        }
        //if option r given server can handle up to 10 connection else just one connection is handled
        if(cmdOps.option_r == 1){
            numConnections = 10;
        }else{
            numConnections = 1;
        }
        //for debugging purpose to see if things are parsed correctly
        if(cmdOps.option_v){
            fprintf(stderr,"number of connections:%d",numConnections);
        }

        if (listen(socketServer, 20) == -1) {
            if(cmdOps.option_v){
                fprintf(stderr,"listen failure\n");
            }
            if(close(socketServer) < 0){
                if(cmdOps.option_v) {
                    fprintf(stderr, "socket closing error\n");
                }
            }
            exit(1);
        }

        //create a thread for closing the progrma when there are no more clients connected given that -k is not provided
        if(cmdOps.option_k == 0){
            if(pthread_create(&connectionCloser,NULL, closeServerUponAllConectionsLeft, &socketServer) < 0){
                if(cmdOps.option_v) {
                    fprintf(stderr, "all connections closed thread creation error\n");
                }
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
                    if(cmdOps.option_v == 1) {
                        fprintf(stderr, "waiting for a connection\n");
                    }
                    sin_size = sizeof their_addr;
                    //accept the connection and set the fd of the client
                    connectionFD = accept(socketServer,(struct sockaddr *)&their_addr, &sin_size);
                    if(connectionFD == -1) {
                        if (cmdOps.option_v) {
                            fprintf(stderr, "%s", "connection denied\n");
                        }
                        continue;
                    }
                    if(cmdOps.option_v == 1) {
                        fprintf(stderr, "accpeted a connection\n");
                    }
                    //create thread for each incoming connection
                    if(pthread_create(&threads[i], NULL, threadHandler, &connectionFD) < 0){
                        if(cmdOps.option_v) {
                            fprintf(stderr, "%s", "thread creation for client error\n");
                        }
                    } else {
                        inet_ntop(their_addr.ss_family,
                                  get_in_addr((struct sockaddr *)&their_addr),
                                  s, sizeof s);
                        if(cmdOps.option_v == 1) {
                            fprintf(stderr, "server: got connection from %s\n", s);
                            fprintf(stderr, "port is %d\n",
                                    ntohs(get_in_port((struct sockaddr *) &their_addr)));
                        }
                        connections[i].inUse = TRUE;
                        connections[i].socketFD = connectionFD;
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
}


int main(int argc, char **argv) {

  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  //for debugging purposes let user know what options they have specified that  are properly parsed
  if(cmdOps.option_v == 1) {
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
  //run program only if parsing was done properly and no options contradict one another
  if(retVal != PARSE_OK){
        fprintf(stderr,"parsing error\n");
        exit(0);
  }
  dashROption = cmdOps.option_r;
  dashKOption = cmdOps.option_k;
  //option l means to run as server or else run as client
  if(cmdOps.option_l == 1){
      if(cmdOps.option_v == 1){
          fprintf(stderr,"acting as server\n");
      }
      actAsServer(cmdOps);
      exit(0);
  }else {
      if(cmdOps.option_v == 1){
          fprintf(stderr,"acting as client\n");
      }
      actAsClient(cmdOps);
      exit(0);
  }

  exit(0);
}


