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

struct pollfd pfds[12];
int number_fd = 10;
int fd_count = 0;
char buff[1024];
bool dashRoption;
bool closeAllConnections;

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

void reader(){
    char line[1024];
    memset(&line,0, sizeof line);
    int len = sizeof line;
    char c;
    while(fgets(line, 1024, stdin) != NULL){
        for (int i = 2; i < 12; i++) {
            if (pfds[i].fd != -1) {
                if (send(pfds[i].fd, line, len, 0) == -1) {
                } else {
                }
            }
        }

        int linelength = strlen(line);
        if(linelength > 0 && line[linelength-1] == '\n'){
            break;
        }

        memset(&line,0, sizeof line);
    }
    fprintf(stderr,"out of while loop\n");
}

void actAsClient(struct commandOptions cmdOps){
    int socketClient;
    socketClient = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in cli_addr;
    struct sockaddr_in remote_addr;
    if (socketClient == -1) {
        fprintf(stderr, "%s", "socket creation failed...\n");
    }else{
        fprintf(stderr, "%s", "Socket successfully created..\n");
    }

    memset(&cli_addr, 0, sizeof cli_addr);
    memset(&remote_addr, 0, sizeof remote_addr);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    remote_addr.sin_family = AF_INET;

    if(cmdOps.hostname){
        fprintf(stderr,"provided remote hostname\n");
        struct hostent * hostnm = gethostbyname(cmdOps.hostname);
        // inet_ntoa(*(long*)host->h_addr_list[0]));
        remote_addr.sin_addr.s_addr =  *(long*)hostnm->h_addr_list[0];
    }else{
        fprintf(stderr,"please provide hostname");
    }
    if(cmdOps.port != 0){
        fprintf(stderr,"provided remote port\n");
        remote_addr.sin_port = htons(cmdOps.port);
    }else{
        fprintf(stderr,"missing remote port\n");
    }

    if(cmdOps.option_p != 0){
        fprintf(stderr,"source port provided\n");
        cli_addr.sin_port = htons(cmdOps.source_port);
    }

    if (bind(socketClient, (struct sockaddr*) &cli_addr, sizeof(cli_addr) == 0)) {
        fprintf(stderr, "%s", "Binded Correctly\n");
    }else {
        fprintf(stderr, "%s", "Unable to bind\n");
    }

    if(connect(socketClient, (struct sockaddr * ) &remote_addr, sizeof(remote_addr)) != -1){
        fprintf(stderr,"succesfully connected\n");
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
                poll_count = poll(clientPfds, clientfd_count, cmdOps.timeout);
                fprintf(stderr,"provided time\n");
            }else{
                poll_count = poll(clientPfds, clientfd_count, -1);
                fprintf(stderr,"didn't provided time\n");
            }
            if (poll_count == -1) {
                fprintf(stderr, "poll count is -1");
                exit(1);
            } else if (poll_count == 0) {
                fprintf(stderr, "timeout occured");
                close(socketClient);
                exit(0);
            } else {
                if (clientPfds[0].revents & POLLIN) {
                    fprintf(stderr,"fsdfsdf");
                    char buf[100];
                    memset(&buf[0], 0, sizeof(buf));
                    fprintf(stderr, "%s", "Enter a message: \n");

                    while (fgets(buf, sizeof(buf), stdin) != NULL) {
                        int length = strlen(buf);
                        while (length > 0) {
                            int i = send(socketClient, buf, length, 0);
                            length -= i;
                            buf[i] = '\0';
                            printf("client: sent %s", buf);
                        }
                        break;
                        memset(&buf[0], 0, sizeof(buf));
                    }
                } else if (clientPfds[1].revents & POLLIN) {
                    fprintf(stderr,"got message");
                    memset(&buff,0, sizeof(buff));
                    int nbytes = recv(clientPfds[1].fd, buff, sizeof buff, 0);
                    fprintf(stderr, buff);
                    memset(&buff,0, sizeof(buff));
                }
            }
        }
    }else{
        fprintf(stderr,"connection failure\n");
    }
    fprintf(stderr,"outside\n");
}

void actAsServer(unsigned int port){
    int socketServer;
    struct sockaddr_in serverAddr;
    socklen_t serverAsClientaddr_size = sizeof serverAddr;
    //creating the socket
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    //setting properties of teh serverAddr
    memset(&serverAddr, 0, sizeof serverAddr);
    memset(&pfds, 0, sizeof pfds);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(port != 0) {
        fprintf(stderr,"port given: %d",port);
        serverAddr.sin_port = htons((int)port);
    }


    //if binding port with the socket is successful
    if(bind(socketServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != -1){
        fprintf(stderr,"%s", "succesfully binded\n");

        //list of connections
        listen(socketServer,20);

        pfds[0].fd = socketServer;
        pfds[0].events = POLLIN;
        pfds[1].fd = STDIN_FILENO;
        pfds[1].events = POLLIN;
        for(int i= 2; i< 12; i++){
            pfds[i].fd = -1;
        }
        fd_count = 2;

        while(1) {
            if(closeAllConnections){
                exit(0);
            }
            int poll_count = poll(pfds, fd_count, -1);
            if(closeAllConnections){
                exit(0);
            }
            fprintf(stderr, "after poll count");
            int connectSocket;
            struct addrinfo clientAddr;
            socklen_t addr_size = sizeof clientAddr;

            if (poll_count == -1) {
                fprintf(stderr, "poll count is -1");
                exit(1);
            } else if (poll_count == 0) {
                fprintf(stderr, "timeout occured");
            } else {
                for (int i = 0; i < 12; i++) {
                    if (pfds[i].fd == -1) {
                        fprintf(stderr, "continue most top\n");
                        continue;
                    }
                    if (pfds[i].revents & POLLIN) {
                        fprintf(stderr, "poll in happened\n");
                        if (pfds[i].fd == STDIN_FILENO) {
                            reader();
                        } else if (pfds[i].fd == socketServer) {
                            fprintf(stderr, "socket server got polled\n");
                            if (fd_count < 13) {
                                fprintf(stderr, "stuck waiting for conneciton 1\n");
                                connectSocket = accept(socketServer, (struct sockaddr *) &clientAddr, &addr_size);
                                fprintf(stderr, "stuck waiting for connection 2\n");
                                fprintf(stderr,"connetion socket:%d\n", connectSocket);
                                if (connectSocket != -1) {
                                    fprintf(stderr,"connection socket is not -1");
                                    for (int j = 2; j < 12; j++) {
                                        fprintf(stderr,"pfds j is: %d\n",pfds[j].fd);
                                        if (pfds[j].fd < 0) {
                                            fprintf(stderr, "adding connection:%d\n", j);
                                            pfds[j].fd = connectSocket;
                                            pfds[j].events = POLLIN;
                                            fd_count++;
                                            break;
                                        }
                                    }
                                    fprintf(stderr,"finished adding\n");
                                }
                            }
                        } else {
                            fprintf(stderr, "pfd[%d] has data to send\n", i);
                            int nbytes = recv(pfds[i].fd, buff, sizeof buff, 0);
                            int sender_fd = pfds[i].fd;
                            if (nbytes == 0) {
                                fprintf(stderr, "client:%d, disconnected\n",i);
                                pfds[i].fd = -1;
                                fd_count--;
                                if(dashRoption){
                                    if(fd_count == 2){
                                        fprintf(stderr,"all clients disconnected, closing socket \n");
                                        pfds[0].fd = -1;
                                        pfds[1].fd = -1;
                                        closeAllConnections = TRUE;
                                        close(socketServer);
                                        break;
                                    }
                                } else {
                                    continue;
                                }
                            } else {
                                fprintf(stderr,"this is input %s",buff);
                                for (int z = 2; z < 12; z++) {
                                    fprintf(stderr, "trying to send to others\n");
                                    if (pfds[z].fd != -1 && pfds[z].fd != sender_fd) {
                                        if (send(pfds[z].fd, buff, nbytes, 0) == -1) {
                                            fprintf(stderr, "send failure\n");
                                        } else {
                                            fprintf(stderr, "send success\n");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }else{
        fprintf(stderr,"%s","server: bind failure\n");
        close(socketServer);
    }
}

int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  dashRoption = cmdOps.option_r;
  if(cmdOps.option_l == 1){
      fprintf(stderr,"acting as server");
      actAsServer(cmdOps.port);
  }else{
      fprintf(stderr,"acting as client\n");
      actAsClient(cmdOps);
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
