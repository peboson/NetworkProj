#ifndef _TCPUDP_H_
#define _TCPUDP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define __WITH_TROLL


#define SERVER_PUBLIC_PORT "8000"
#define SERVER_LOCAL_PORT "8001"

#define CLIENT_PUBLIC_PORT "8100"
#define CLIENT_LOCAL_PORT "8101"

#define TCPD_MSG_LEN 8
#define BUF_LEN 1000
#define MAX_PATH 260

#ifdef __WITH_TROLL
#define TROLL_PORT "8081"
#include "Troll/troll.h"
struct troll_message_t{
    struct sockaddr_in header;
    char body[BUF_LEN];
};
#endif

void init_tcpudp(const char * role);
int SOCKET(int domain, int type, int protocol);
int BIND(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int LISTEN(int sockfd, int backlog);
int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t RECV(int sockfd, void *buf, size_t len, int flags);
int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t SEND(int sockfd, const void *buf, size_t len, int flags);
int CLOSE(int fd);


#endif
