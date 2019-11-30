#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "commonProto.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <sys/poll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

struct pollfd pfds[12]; //aray of pollfds server is to handle, with 2 of them including stdin and server's socket
int number_fd = 10; //total number of fds excluding stdin and server's socket;
int fd_count = 0; //current number of fd count
char buff[1024]; //array with which we recv
bool dashRoption; //flag to check if -r is set
int dashKoption; //determines if server should close or not after conections goes to 0
bool closeAllConnections; //flag to check if there are remaining clients connected to the server
int hasAcceptedAtleastOne = 0;
int currentConnections = 0;

int sendall(int s, char *buf, int *len)
{
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

//if stdin is polled read from stdin and send message to all connected clients
void reader(){

    char ch[1024];
    memset(&ch, 0, sizeof ch);
ioifterituertuerituerituieriutieutier
    //keep reading on stdin
    while(read(0, &ch, sizeof(ch)) > 0){
        fprintf(stderr,"reading");
        int len = strlen(ch);
        //don't need to send to stdin or server socket
        for (int i = 2; i < 12; i++) {
            if (pfds[i].fd != -1) {
                fprintf(stderr,"found one not -1");
                if (sendall(pfds[i].fd, ch,&len) == -1) {
                    fprintf(stderr,"stdin read error\n");
                }
            }
        }
        if(ch[len] == '\0'){
            break;
        }
        memset(&ch, 0, sizeof ch);
    }
    fprintf(stderr,"out of while loop\n");
}

void actAsClient(struct commandOptions cmdOps){
    //socket of the client
    int socketClient;
    socketClient = socket(AF_INET, SOCK_STREAM, 0);
    //sockaddr_in of the client itself in case source port needs to be set
    struct sockaddr_in cli_addr, remote_addr;

    //check if creation of socket was succesful or not
    if (socketClient == -1) {
        fprintf(stderr, "%s", "socket creation failed...\n");
        if(close(socketClient) < 0){
            fprintf(stderr,"closing socket failure\n");
        }
    }else{
        fprintf(stderr, "%s", "Socket successfully created..\n");
    }

    memset(&cli_addr, 0, sizeof cli_addr);
    memset(&remote_addr, 0, sizeof remote_addr);

    remote_addr.sin_family = AF_INET;

    if(cmdOps.hostname != NULL){
        fprintf(stderr,"provided remote hostname\n");
        struct hostent * hostnm = gethostbyname(cmdOps.hostname);
        remote_addr.sin_addr.s_addr =  *(long*)hostnm->h_addr_list[0];
    }else{
        if(cmdOps.option_v) {
            fprintf(stderr, "please provide hostname\n");
        }
        close(socketClient);
        exit(0);
    }
    //check if the client has provided a port of the server it wishes to connect to
    //client must provide a server port that it wishes to connect to
    if(cmdOps.port != 0){
        if(cmdOps.port < 1023 || cmdOps.port > 65535){
            fprintf(stderr,"invalid port: reserved \n");
            exit(1);
        }
        fprintf(stderr,"provided remote port\n");
        remote_addr.sin_port = htons(cmdOps.port);
    }else if(cmdOps.port == 0){
        fprintf(stderr,"missing remote port\n");
        if(close(socketClient) < 0){
            fprintf(stderr,"closing socket failure\n");
        }
        exit(1);
    }

    //if client provided source port it wishes to bind its socket with otherwise no need to bind
    if(cmdOps.option_p != 0) {
        if(cmdOps.source_port < 1023 || cmdOps.source_port > 65535){
            if(cmdOps.option_v) {
                fprintf(stderr, "error: reserved ports used for source port\n");
            }
            exit(1);
        }
        fprintf(stderr, "source port provided\n");
        cli_addr.sin_family = AF_INET;
        cli_addr.sin_addr.s_addr = INADDR_ANY;
        fprintf(stderr,"cmdsourceports:%d",cmdOps.source_port);
        cli_addr.sin_port = htons(cmdOps.source_port);
        if (bind(socketClient, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) == 0) {
            fprintf(stderr, "%s", "Binded Correctly\n");
        } else {
            if(cmdOps.option_v) {
                fprintf(stderr, "%s", "Unable to bind\n");
            }
            if(close(socketClient) < 0){
                fprintf(stderr,"closing socket failure\n");
            }
            exit(0);
        }
    }
    //connect client to server
    if(connect(socketClient, (struct sockaddr * ) &remote_addr, sizeof(remote_addr)) != -1){
        struct pollfd clientPfds[2];
        int clientfd_count;
        clientPfds[0].fd = 0;
        clientPfds[0].events = POLLIN;
        clientPfds[1].fd = socketClient;
        clientPfds[1].events = POLLIN;
        clientfd_count = 2;
        int poll_count;
        while(1) {
            if(cmdOps.timeout != 0){
                fprintf(stderr,"again with timeout limit\n");
                poll_count = poll(clientPfds, clientfd_count, cmdOps.timeout*1000);
            }else{
                fprintf(stderr,"again with no timeout limit\n");
                poll_count = poll(clientPfds, clientfd_count, -1);
            }
            if (poll_count == -1) {
                fprintf(stderr, "poll count is -1");
                exit(1);
            } else if (poll_count == 0) {
                fprintf(stderr, "timeout occured");
                close(socketClient);
                exit(0);
            } else {
                //if stdin got polled
                if (clientPfds[0].revents & POLLIN) {
                    char ch[1024];
                    memset(ch, 0, sizeof ch);

                    //keep reading on stdin
                    while(read(0, &ch, sizeof(ch)) > 0)
                    {
                        int len = strlen(ch);
                        //send stdin message to server
                        if (sendall(socketClient, ch,&len) == -1) {
                            fprintf(stderr,"stdin read error\n");
                        }
                        if(ch[len-1] == '\n'){
                            fprintf(stderr,"last char is enter");
                            break;
                        }
                        memset(ch, 0, sizeof ch);
                    }
                    //if socket is polled, meaning client received message from server
                } else if (clientPfds[1].revents & POLLIN) {
                    char ch[1024];
                    memset(&ch, 0, sizeof ch);
                    int n = recv(clientPfds[1].fd, ch, sizeof(ch), 0);
                    if(n == 0){
                        fprintf(stderr,"server disconnection\n");
                        exit(0);
                    } else if(n == -1){
                        fprintf(stderr,"error\n");
                        continue;
                    }else if(n > 0) {
                        fprintf(stderr,"got smt\n");
                        int len = strlen(ch);
                        if (write(1, ch, len) < 0) {
                            fprintf(stderr, "write to stdout error\n");
                        }
                        memset(&ch, 0, sizeof ch);
                    }
                }
            }
        }
    }else{
        if(cmdOps.option_v) {
            fprintf(stderr, "connection error\n");
        }
        if(close(socketClient) < 0){
            fprintf(stderr,"closing socket failure\n");
        }
        exit(0);
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
    int socketServer;
    int numofConnections = 0;
    struct sockaddr_in serverAddr;
    socklen_t serverAsClientaddr_size = sizeof serverAddr;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    //creating the socket
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    //setting properties of teh serverAddr
    memset(&serverAddr, 0, sizeof serverAddr);
//    memset(&pfds, 0, sizeof pfds);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(cmdOps.port != 0) {
        if(cmdOps.port < 1023 || cmdOps.port > 65535){
            fprintf(stderr,"invalid: reserved ports\n");
            exit(1);
        }
        fprintf(stderr,"port given: %d",cmdOps.port);
        serverAddr.sin_port = htons((int)cmdOps.port);
    }else if(cmdOps.port == 0){
        fprintf(stderr,"provide port for server\n");
        exit(1);
    }

    if(cmdOps.option_r == 1){
        //10 connections plus stdin and socket of the server
        numofConnections = 12;
    }else{
        //1 connection plus stdin and socket of the server
        numofConnections = 1;
    }
    fprintf(stderr,"number of connections:%d\n", numofConnections);

    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){

        //list for connections with 20 in backlog queue(random decision could be any number)
        listen(socketServer,20);
        //adding stdin and socket to our array of pfds
        pfds[0].fd = socketServer;
        pfds[0].events = POLLIN;
        pfds[1].fd = STDIN_FILENO;
        pfds[1].events = POLLIN;

        //initializing clients to have fd of -1 so they dont get polled since no connections yet
        for(int i= 2; i< 12; i++){
            pfds[i].fd = -1;
        }
        //fds so far, stdin and socket since no connection has been accepted
        fd_count = 12;

        //keep polling for incoming connections, or if server wrote something to broadcast
        while(1) {
//           fprintf(stderr,"before poll count\n");
//          fprintf(stderr,"fd_count:%d\n", fd_count);
//           fprintf(stderr,"pfds[2].fd:%d\n",pfds[2].fd);
//           fprintf(stderr,"pfds[3].fd:%d\n",pfds[3].fd);
            int poll_count = poll(pfds,fd_count, -1);
//           fprintf(stderr, "after poll count");
            int connectSocket;
            struct addrinfo clientAddr;
            socklen_t addr_size = sizeof clientAddr;

            if (poll_count == -1) {
                fprintf(stderr, "poll count is -1");
                if(close(socketServer)<0){
                    fprintf(stderr,"closing socket failure\n");
                }
                fprintf(stderr,"exiting from here\n");
                exit(1);
            } else if (poll_count == 0) {
                fprintf(stderr, "timeout occured");
                if(close(socketServer)<0){
                    fprintf(stderr,"closing socket failure\n");
                }
                exit(0);
            } else {
//                fprintf(stderr,"boom\n");
                //check if fd triggered a poll event
                for (int i = 0; i < 12; i++) {
                    //there is no connection of this fd yet
                    if (pfds[i].fd == -1) {
                        continue;
                    }
                    //check if this connection or stdin caused the poll event
                    if (pfds[i].revents & POLLIN) {
                        //if poll event was caused by stdin, server wanting to broadcast
                        if (pfds[i].fd == STDIN_FILENO) {
                            fprintf(stderr,"stdin got polled\n");
                            reader();
                            fprintf(stderr,"back from reader\n");
                            //if poll event caused by the server socket, a client wishing to connect
                        } else if (pfds[i].fd == socketServer) {
//                            fprintf(stderr,"socketserver polled\n");
                            if (currentConnections < numofConnections) {
                                fprintf(stderr, "stuck waiting for conneciton 1\n");
                                sin_size = sizeof their_addr;
                                connectSocket = accept(socketServer, (struct sockaddr *)&their_addr, &sin_size);
                                fprintf(stderr, "stuck waiting for connection 2\n");
                                fprintf(stderr,"connetion socket:%d\n", connectSocket);
                                if (connectSocket != -1) {
                                    for (int j = 2; j < 12; j++) {
                                        if (pfds[j].fd < 0) {
                                            fprintf(stderr,"i is:%d\n",i);
                                            fprintf(stderr,"j is:%d\n",j);
                                            inet_ntop(their_addr.ss_family,
                                                      get_in_addr((struct sockaddr *)&their_addr),
                                                      s, sizeof s);
                                            fprintf(stderr,"server: got connection from %s\n", s);
                                            fprintf(stderr,"port is %d\n",ntohs(get_in_port((struct sockaddr *)&their_addr)));
//                                            fprintf(stderr, "ip address:%d\n", getnetent(socketServer));
//                                            fprintf(stderr,"port number:%d\n", getopt(socketServer,NULL,NULL));
                                            pfds[j].fd = connectSocket;
                                            fprintf(stderr,"connection scoket:%d\n",connectSocket);
                                            pfds[j].events = POLLIN;
                                            fprintf(stderr,"fd_count after add:%d\n",fd_count);
                                            currentConnections++;
                                            break;
                                        }
                                    }
                                    hasAcceptedAtleastOne =1;
                                    fprintf(stderr,"finished adding\n");
                                    continue;
                                }
                            }
                        } else {
                            //a client sent a message, server must retransmit to all other clients
                            int nbytes = recv(pfds[i].fd, buff, sizeof buff, 0);
                            int sender_fd = pfds[i].fd;
                            //a client has disconnected, server must decrement fd count and set original client fd
                            //to -1 to free up space for other clients to connect
                            if (nbytes == 0) {
                                fprintf(stderr, "client:%d, disconnected\n",i);
                                pfds[i].fd = -1;
                                currentConnections--;
                                fprintf(stderr,"currentConnections after disconnection:%d\n", currentConnections);
                                if(dashKoption == 0){
                                    if(currentConnections == 0){
                                        pfds[0].fd = -1;
                                        pfds[1].fd = -1;
                                        fprintf(stderr,"exiting from close\n");
                                        close(socketServer);
                                        exit(0);
                                    }
                                }
                            } else if(nbytes > 0) {
                                //retransmit client's message to all other connected clients and write to server's
                                //stdout
                                int len = strlen(buff);
                                write(1,buff,len);
                                for (int z = 2; z < 12; z++) {
                                    if (pfds[z].fd != -1 && pfds[z].fd != sender_fd) {
                                        fprintf(stderr,"found 1");
                                        if (sendall(pfds[z].fd, buff, &len) == -1) {
                                            fprintf(stderr, "send failure\n");
                                        } else {
                                            fprintf(stderr, "send success\n");
                                        }
                                    }
                                }
                                memset(&buff[0],0, sizeof(buff));
                            }else{
                                fprintf(stderr,"here\n");
                            }
                        }
                    }
                }
            }
        }
    }else{
        fprintf(stderr,"else case\n");
        if(cmdOps.option_v) {
            fprintf(stderr, "server: bind failure\n");
        }
        if(close(socketServer)< 0){
            fprintf(stderr,"closing  socket error\n");
        }
    }
}

int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  if(cmdOps.option_v) {
      fprintf(stderr, "option-k:%d\n", cmdOps.option_k);
      fprintf(stderr, "option-l:%d\n", cmdOps.option_l);
      fprintf(stderr, "option-v:%d\n", cmdOps.option_v);
      fprintf(stderr, "option-r:%d\n", cmdOps.option_r);
      fprintf(stderr, "option-p:%d\n", cmdOps.option_p);
      fprintf(stderr, "option-source port:%d\n", cmdOps.source_port);
      fprintf(stderr, "co-timeout:%d\n", cmdOps.timeout);
      fprintf(stderr, "option-hostname:%d\n", cmdOps.hostname);
      fprintf(stderr, "option-port:%d\n", cmdOps.port);
  }
  dashRoption = cmdOps.option_r;
  dashKoption = cmdOps.option_k;
    //run program only if parsing was done properly and no options contradict one another
  if(retVal != PARSE_OK){
        fprintf(stderr,"parsing error\n");
        exit(0);
  }
    //option l means to run as server or else run as client
  if(cmdOps.option_l == 1){
      if(cmdOps.option_v) {
          fprintf(stderr, "acting as server");
      }
      actAsServer(cmdOps);
  } else {
      if(cmdOps.option_v) {
          fprintf(stderr, "acting as client\n");
      }
      actAsClient(cmdOps);
  }
  exit(0);
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
