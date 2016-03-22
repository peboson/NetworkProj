/* File: tcpudp.c
 * Author: Jason Song.988
 * Description: CSE5462 Lab3
 *     The description of TCP interface
 */
#ifndef _TCPUDP_H_
#define _TCPUDP_H_

#include "Header.h"

void InitTcpUdp(const char * TcpdPort);
int SOCKET(int domain, int type, int protocol);
int BIND(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int LISTEN(int sockfd, int backlog);
int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t RECV(int sockfd, void *buf, size_t len, int flags);
int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t SEND(int sockfd, const void *buf, size_t len, int flags);
int CLOSE(int sockfd);


#endif
