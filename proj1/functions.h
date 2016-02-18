#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

//make UDP socket from TCP command
int SOCKET(int family, int type, int protocol);

//send msg to tcpd
//int SEND(int sockfd, const void *msg, int len, int flags);

//recieve buf from tcpd
int RECV(int sockfd, void *buf, int len, int flags);

//NULL function for lab3
int CONNECT(int sockfd, struct sockaddr *serv_addr, int addrlen);

//NULL function for lab3
int ACCEPT(int sockfd, void *peer, int *addrlen);

//close socket
int CLOSE (int sock);


#endif // FUNCTIONS_H_
