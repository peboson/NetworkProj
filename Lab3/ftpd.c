/* Example: server.c receiving and sending datagrams on system generated port in UDP */
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "tcpudp.h"

int sock;
char buf[BUF_LEN];
struct sockaddr_in tcpd_name;
socklen_t tcpd_name_len;
char roleport[MAX_PATH];

void tcpd_SOCKET(struct sockaddr_in front_addr,socklen_t front_addr_len);
void tcpd_BIND(struct sockaddr_in front_addr,socklen_t front_addr_len);
void tcpd_RECV(struct sockaddr_in front_addr,socklen_t front_addr_len);
void tcpd_CONNECT(struct sockaddr_in front_addr,socklen_t front_addr_len);
void tcpd_SEND(struct sockaddr_in front_addr,socklen_t front_addr_len);
int main(int argc, char *argv[]) /* server program called with no argument */
{

    if(argc==2){
        strcpy(roleport,argv[1]);
    }else{
        fprintf(stderr,"USAGE: ftpd server|client|port\n");
        exit(1);
    }

    /* create socket for connecting to server */
    sock = socket(AF_INET, SOCK_DGRAM,0);
    if(sock < 0) {
        perror("opening datagram socket");
        exit(2);
    }

    /* create name with parameters and bind name to socket */
    tcpd_name.sin_family = AF_INET;
    if(strcmp(roleport,"server")==0){
        tcpd_name.sin_port = htons(atoi(SERVER_LOCAL_PORT));
    }else if(strcmp(roleport,"client")==0){
        tcpd_name.sin_port = htons(atoi(CLIENT_LOCAL_PORT));
    }else{
        tcpd_name.sin_port = htons(atoi(roleport));
    }
    tcpd_name.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) < 0) {
        perror("error binding socket");
        exit(2);
    }
    tcpd_name_len=sizeof(tcpd_name);
    /* Find assigned port value and print it for client to use */
    if(getsockname(sock, (struct sockaddr *)&tcpd_name, &tcpd_name_len) < 0){
        perror("error getting sock name");
        exit(3);
    }
    printf("Server waiting on port # %d\n", ntohs(tcpd_name.sin_port));

    while(1){
        struct sockaddr_in front_addr;
        socklen_t front_addr_len;
        char tcpd_msg[TCPD_MSG_LEN];
        /* waiting for connection from client on name and print what client sends */
        front_addr_len= sizeof(front_addr);
        if(recvfrom(sock, tcpd_msg, TCPD_MSG_LEN, MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
            perror("error receiving"); 
            exit(4);
        }
        printf("Server receives: %s\n", tcpd_msg);
        if(strcmp(tcpd_msg,"SOCKET")==0){
            tcpd_SOCKET(front_addr,front_addr_len);
        }else if(strcmp(tcpd_msg,"BIND")==0){
            tcpd_BIND(front_addr,front_addr_len);
        }else if(strcmp(tcpd_msg,"RECV")==0){
            tcpd_RECV(front_addr,front_addr_len);
        }else if(strcmp(tcpd_msg,"CONNECT")==0){
            tcpd_CONNECT(front_addr,front_addr_len);
        }else if(strcmp(tcpd_msg,"SEND")==0){
            tcpd_SEND(front_addr,front_addr_len);
        }
    }
    //     if(recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //         perror("error receiving"); 
    //         exit(4);
    //     }
    //     printf("Server receives: %s\n", buf);

    //  server sends something back using the same socket 
    // bzero(srv_buf, MAX_MES_LEN);
    // strcpy(srv_buf, "Hello back in UDP from server");
    // printf("Server sends:    %s\n", srv_buf);
    // if(sendto(sock, srv_buf, buflen, 0, (struct sockaddr *)&name, namelen) < 0) {
    // perror("sending datagram");
    // exit(5);
    // }

    /* server terminates connection, closes socket, and exits */
    // close(sock);
    exit(0);
}




void tcpd_SOCKET(struct sockaddr_in front_addr,socklen_t front_addr_len){
    // int domain;
    // int type;
    // int protocol;

    // bzero(buf, sizeof(int));
    // front_addr_len= sizeof(front_addr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //     perror("error receiving SOCKET domain"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&domain,sizeof(int));

    // bzero(buf, sizeof(int));
    // front_addr_len= sizeof(front_addr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //     perror("error receiving SOCKET type"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&type,sizeof(int));

    // bzero(buf, sizeof(int));
    // front_addr_len= sizeof(front_addr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //     perror("error receiving SOCKET protocol"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&protocol,sizeof(int));

    printf("SOCKET\n");
    // printf("SOCKET domain:%d, type:%d, protocol:%d\n",domain,type,protocol);

    int ret_sock=socket(AF_INET, SOCK_DGRAM,0);
    if(ret_sock < 0) {
        perror("opening datagram socket");
        exit(2);
    }

    printf("SOCKET return %d\n",ret_sock);

    if(sendto(sock, (char *)&ret_sock, sizeof(int), 0, (struct sockaddr *)&front_addr, sizeof(front_addr)) <0) {
        perror("error sending SOCKET return");
        exit(4);
    }
}


void tcpd_BIND(struct sockaddr_in front_addr,socklen_t front_addr_len){
    int sockfd;
    struct sockaddr_in addr;
    // socklen_t addrlen;

    bzero(buf, sizeof(int));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving BIND sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(struct sockaddr_in));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(struct sockaddr_in),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving BIND addr"); 
        exit(4);
    }
    bcopy(buf,(char *)&addr,sizeof(struct sockaddr_in));

    // bzero(buf, sizeof(socklen_t));
    // front_addr_len= sizeof(front_addr);
    // if(recvfrom(sock, buf, sizeof(socklen_t),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //     perror("error receiving BIND addrlen"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&addrlen,sizeof(socklen_t));

    printf("BIND sockfd:%d, addr.port:%hu, addr.in_addr:%lu\n",sockfd,addr.sin_port,(unsigned long)addr.sin_addr.s_addr);
    // printf("SOCKET sockfd:%d, addr.port:%hu, addr.in_addr:%lu, addrlen:%d\n",sockfd,addr.sin_port,addr.sin_addr,addrlen);

    int ret_bind=bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret_bind < 0) {
        perror("error binding socket");
        exit(2);
    }

    printf("BIND return %d\n",ret_bind);
    
    if(sendto(sock, (char *)&ret_bind, sizeof(int), 0, (struct sockaddr *)&front_addr, sizeof(front_addr)) <0) {
        perror("error sending BIND return");
        exit(4);
    }
}

void tcpd_RECV(struct sockaddr_in front_addr,socklen_t front_addr_len){
    int sockfd;
    // void *buf;
    size_t len;
    // int flags;

    bzero(buf, sizeof(int));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving RECV sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(size_t));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(size_t),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving RECV len"); 
        exit(4);
    }
    bcopy(buf,(char *)&len,sizeof(size_t));

    // bzero(buf, sizeof(int));
    // front_addr_len= sizeof(front_addr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //     perror("error receiving RECV flags"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&flags,sizeof(int));

    printf("RECV sockfd:%d, len:%zu\n",sockfd,len);
    // printf("RECV sockfd:%d, len:%zu, flags:%d\n",sockfd,len,flags);

    if(BUF_LEN<len){
        perror("error request length larger than buffer");
        exit(5);
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    client_addr_len=sizeof(client_addr);
    ssize_t ret_recv=recvfrom(sockfd, buf, len, MSG_WAITALL, (struct sockaddr *)&client_addr, &client_addr_len);
    if(ret_recv < 0) {
        perror("error receiving RECV");
        exit(5);
    }

    printf("RECV return %zd\n",ret_recv);
    
    if(sendto(sock, buf, ret_recv, 0, (struct sockaddr *)&front_addr, sizeof(front_addr)) <0) {
        perror("error sending RECV return");
        exit(4);
    }
}

void tcpd_CONNECT(struct sockaddr_in front_addr,socklen_t front_addr_len){
    int sockfd;
    struct sockaddr_in addr;
    // socklen_t addrlen;

    bzero(buf, sizeof(int));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving CONNECT sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(struct sockaddr_in));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(struct sockaddr_in),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving CONNECT addr"); 
        exit(4);
    }
    bcopy(buf,(char *)&addr,sizeof(struct sockaddr_in));

    // bzero(buf, sizeof(socklen_t));
    // front_addr_len= sizeof(front_addr);
    // if(recvfrom(sock, buf, sizeof(socklen_t),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //     perror("error receiving CONNECT addrlen"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&addrlen,sizeof(socklen_t));

    printf("CONNECT sockfd:%d, addr.port:%hu, addr.in_addr:%lu\n",sockfd,addr.sin_port,(unsigned long)addr.sin_addr.s_addr);
    // printf("CONNECT sockfd:%d, addr.port:%hu, addr.in_addr:%lu, addrlen:%d\n",sockfd,addr.sin_port,addr.sin_addr,addrlen);

    int ret_connect=connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret_connect < 0) {
        perror("error connecting socket");
        exit(2);
    }

    printf("CONNECT return %d\n",ret_connect);
    
    if(sendto(sock, (char *)&ret_connect, sizeof(int), 0, (struct sockaddr *)&front_addr, sizeof(front_addr)) <0) {
        perror("error sending CONNECT return");
        exit(4);
    }
}

void tcpd_SEND(struct sockaddr_in front_addr,socklen_t front_addr_len){
    int sockfd;
    // void *buf;
    size_t len;
    // int flags;

    bzero(buf, sizeof(int));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving SEND sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(size_t));
    front_addr_len= sizeof(front_addr);
    if(recvfrom(sock, buf, sizeof(size_t),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving SEND len"); 
        exit(4);
    }
    bcopy(buf,(char *)&len,sizeof(size_t));

    // bzero(buf, sizeof(int));
    // front_addr_len= sizeof(front_addr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
    //     perror("error receiving send flags"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&flags,sizeof(int));

    printf("SEND sockfd:%d, len:%zu\n",sockfd,len);
    // printf("SEND sockfd:%d, len:%zu, flags:%d\n",sockfd,len,flags);

    if(BUF_LEN<len){
        perror("error request length larger than buffer");
        exit(5);
    }

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len;
    peer_addr_len=sizeof(peer_addr);
    if(getpeername(sockfd,(struct sockaddr *)&peer_addr, &peer_addr_len)<0){
        perror("error getting peer addr");
        exit(5);
    }
    printf("SEND sockfd:%d, peer_addr.port:%hu, peer_addr.in_addr:%lu\n",sockfd,peer_addr.sin_port,(unsigned long)peer_addr.sin_addr.s_addr);

    if(recvfrom(sock, buf, len, MSG_WAITALL, (struct sockaddr *)&front_addr, &front_addr_len) < 0) {
        perror("error receiving SEND buf"); 
        exit(4);
    }

    ssize_t ret_send=sendto(sockfd, buf, len, MSG_WAITALL, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    if(ret_send < 0) {
        perror("error sending buffer");
        exit(5);
    }

    printf("SEND return %zd\n",ret_send);
    
    if(sendto(sock, (char *)&ret_send, sizeof(int), 0, (struct sockaddr *)&front_addr, sizeof(front_addr)) <0) {
        perror("error sending SEND return");
        exit(4);
    }
}
