/* File: tcpudp.c
 * Author: Jason Song.988
 * Description: CSE5462 Lab3
 *     The implementation of wrapper of TCP interface
 */

#include "stdafx.h"
#include "tcpudp.h"

int sock=-1;
int buflen;
struct sockaddr_in tcpd_name;
struct hostent *tcpd_hp, *gethostbyname();
char buf[BUF_LEN];

void init_tcpudp(const char * role){
    /* create socket for connecting to tcp deamon */
    sock = socket(AF_INET, SOCK_DGRAM,0);
    if(sock < 0) {
        perror("opening datagram socket");
        exit(2);
    }
    /* construct tcpd_name for connecting to tcpd */
    tcpd_name.sin_family = AF_INET;
    if(strcmp(role,"SERVER")==0){
        tcpd_name.sin_port = htons(atoi(SERVER_LOCAL_PORT));
    }else if(strcmp(role,"CLIENT")==0){
        tcpd_name.sin_port = htons(atoi(CLIENT_LOCAL_PORT));
    }
    /* convert hostname to IP address and enter into name */
    tcpd_hp = gethostbyname("localhost");
    if(tcpd_hp == 0) {
        perror("localhost: unknown host\n");
        exit(3);
    }
    bcopy((char *)tcpd_hp->h_addr, (char *)&tcpd_name.sin_addr, tcpd_hp->h_length);
}

int SOCKET(int domain, int type, int protocol){
    /* deamon tcp function call of SOCKET */
    printf("SOCKET domain:%d, type:%d, protocol:%d\n",domain,type,protocol);
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    int ret;

    /* send message to deamon */
    char tcpd_msg[TCPD_MSG_LEN]="SOCKET";
    if(sendto(sock, tcpd_msg, TCPD_MSG_LEN, 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending SOCKET tcpd_msg");
        exit(4);
    }

    /* send SOCKET arguments to tcpd */
    // if(sendto(sock, (char *)&domain, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
    //     perror("error sending SOCKET domain");
    //     exit(4);
    // }

    // if(sendto(sock, (char *)&type, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
    //     perror("error sending SOCKET type");
    //     exit(4);
    // }

    // if(sendto(sock, (char *)&protocol, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
    //     perror("error sending SOCKET protocol");
    //     exit(4);
    // }

    /* waiting for deamon to respond using the same socket */
    bzero(buf, sizeof(int));
    server_addr_len=sizeof(server_addr);
    if(recvfrom(sock, buf, sizeof(int), MSG_WAITALL, (struct sockaddr *)&server_addr, &server_addr_len ) < 0) {
        perror("error receiving SOCKET return");
        exit(5);
    }
    bcopy(buf,(char *)&ret,sizeof(int));

    printf("SOCKET return: %d\n",ret);
    return ret;
}

int BIND(int sockfd, const struct sockaddr *addr,socklen_t addrlen){
    /* deamon tcp function call of BIND */
    printf("BIND sockfd:%d, addr.port:%hu, addr.in_addr:%lu, addrlen:%d\n",sockfd,((const struct sockaddr_in *)addr)->sin_port,(unsigned long)((const struct sockaddr_in *)addr)->sin_addr.s_addr,addrlen);
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    int ret;

    /* send message to deamon */
    char tcpd_msg[TCPD_MSG_LEN]="BIND";
    if(sendto(sock, tcpd_msg, TCPD_MSG_LEN, 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending BIND tcpd_msg");
        exit(4);
    }

    /* send BIND arguments to tcpd */
    if(sendto(sock, (char *)&sockfd, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending BIND sockfd");
        exit(4);
    }

    if(sendto(sock, (char *)addr, sizeof(struct sockaddr_in), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending BIND addr");
        exit(4);
    }

    // if(sendto(sock, (char *)&addrlen, sizeof(addrlen), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
    //     perror("error sending BIND addrlen");
    //     exit(4);
    // }

    /* waiting for server to respond using the same socket */
    bzero(buf, sizeof(int));
    server_addr_len=sizeof(server_addr);
    if(recvfrom(sock, buf, sizeof(int), MSG_WAITALL, (struct sockaddr *)&server_addr, &server_addr_len ) < 0) {
        perror("error receiving BIND return");
        exit(5);
    }
    bcopy(buf,(char *)&ret,sizeof(int));

    printf("BIND return: %d\n",ret);
    return ret;
}

int LISTEN(int sockfd, int backlog){
    /* do nothing */
    return 0;
}

int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    /* return the socket file descriptor on the deamon */
    return sockfd;
}

ssize_t RECV(int sockfd, void *buffer, size_t len, int flags){
    printf("RECV sockfd:%d, len:%zu, flags:%d\n",sockfd,len,flags);
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    ssize_t ret;

    /* send message to deamon */
    char tcpd_msg[TCPD_MSG_LEN]="RECV";
    if(sendto(sock, tcpd_msg, TCPD_MSG_LEN, 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending RECV tcpd_msg");
        exit(4);
    }

    if(sendto(sock, (char *)&sockfd, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending RECV sockfd");
        exit(4);
    }

    if(sendto(sock, (char *)&len, sizeof(size_t), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending RECV len");
        exit(4);
    }

    // if(sendto(sock, (char *)&flags, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
    //     perror("error sending RECV flags");
    //     exit(4);
    // }

    /* wait server to response */
    server_addr_len=sizeof(server_addr);
    ret=recvfrom(sock, buffer, len, MSG_WAITALL, (struct sockaddr *)&server_addr, &server_addr_len );
    // if(ret < 0) {
    //     perror("error receiving RECV");
    //     exit(5);
    // }

    printf("RECV return %zd\n",ret);
    return ret;
}

int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    printf("CONNECT sockfd:%d, addr.port:%hu, addr.in_addr:%lu, addrlen:%d\n",sockfd,((const struct sockaddr_in *)addr)->sin_port,(unsigned long)((const struct sockaddr_in *)addr)->sin_addr.s_addr,addrlen);
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    int ret;

    /* send message to deamon */
    char tcpd_msg[TCPD_MSG_LEN]="CONNECT";
    if(sendto(sock, tcpd_msg, TCPD_MSG_LEN, 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending CONNECT tcpd_msg");
        exit(4);
    }

    if(sendto(sock, (char *)&sockfd, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending CONNECT sockfd");
        exit(4);
    }

    if(sendto(sock, (char *)addr, sizeof(struct sockaddr_in), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending CONNECT addr");
        exit(4);
    }

    // if(sendto(sock, (char *)&addrlen, sizeof(addrlen), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
    //     perror("error sending BIND addrlen");
    //     exit(4);
    // }

    /* send the name that connected to to record name on deamon */
    bzero(buf, sizeof(int));
    server_addr_len=sizeof(server_addr);
    if(recvfrom(sock, buf, sizeof(int), MSG_WAITALL, (struct sockaddr *)&server_addr, &server_addr_len ) < 0) {
        perror("error receiving CONNECT return");
        exit(5);
    }
    bcopy(buf,(char *)&ret,sizeof(int));

    printf("CONNECT return: %d\n",ret);
    return ret;
}
ssize_t SEND(int sockfd, const void *buffer, size_t len, int flags){
    printf("SEND sockfd:%d, len:%zu, flags:%d\n",sockfd,len,flags);
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    ssize_t ret;

    /* send message to deamon */
    char tcpd_msg[TCPD_MSG_LEN]="SEND";
    if(sendto(sock, tcpd_msg, TCPD_MSG_LEN, 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending SEND tcpd_msg");
        exit(4);
    }

    if(sendto(sock, (char *)&sockfd, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending SEND sockfd");
        exit(4);
    }

    if(sendto(sock, (char *)&len, sizeof(size_t), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending SEND len");
        exit(4);
    }

    // if(sendto(sock, (char *)&flags, sizeof(int), 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
    //     perror("error sending SEND flags");
    //     exit(4);
    // }

    /* send out package to deamon */
    if(sendto(sock, buffer, len, 0, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) <0) {
        perror("error sending SEND buffer");
        exit(4);
    }

    /* get the send result */
    bzero(buf, sizeof(ssize_t));
    server_addr_len=sizeof(server_addr);
    if(recvfrom(sock, buf, sizeof(ssize_t), MSG_WAITALL, (struct sockaddr *)&server_addr, &server_addr_len ) < 0) {
        perror("error receiving SEND return");
        exit(5);
    }
    bcopy(buf,(char *)&ret,sizeof(ssize_t));

    printf("SEND return: %zd\n",ret);
    return ret;
}


int CLOSE(int fd){
    /* do nothing */
    return 0;
}
