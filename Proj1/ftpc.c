/* client.c using TCP */
#include "stdafx.h"

/* Client for connecting to Internet stream server waiting on port 1040 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

/* client program called with host name where server is run */
int main(int argc, char *argv[])
{
    GlobalInitialize();
    int sock; /* initial socket descriptor */
    int r_val; /* returned value from read */
    int w_val; /* returned value from write */
    struct sockaddr_in sin_addr; /* structure for socket name setup */

    char server_name[MAX_PATH];
    char file_name[MAX_PATH];
    char port[8]=SERVER_PUBLIC_PORT; /* default port */
    unsigned char buf[BUF_LEN]; /* message to sent to server */
    struct hostent *hp; /* server host */

    FILE *send_file;

    /* test command line format */
    if(argc==4){
        /* fetch command line auguments */
        strcpy(server_name,argv[1]);
        strcpy(port,argv[2]);
        strcpy(file_name,argv[3]);
    }else{
        fprintf(stderr,"Incorrect command line format.\n");
        fprintf(stderr,"USAGE: ftpc remote_IP remote_port local_file_to_transfer\n");
        exit(1);
    }

    init_tcpudp("CLIENT");

    /* get send file size without open it */
    unsigned long file_size = -1,file_left=-1;
    struct stat file_stat;
    if(stat(file_name,&file_stat)>=0)
        file_size=file_stat.st_size;
    fprintf(stdout,"Size of file is %lu\n",file_size);

    /* get send file descriptor */
    send_file=fopen(file_name,"rb");

    /* initialize socket connection in unix domain */
    if((sock = SOCKET(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("error openting datagram socket");
        exit(1);
    }

    /* fetch server host via host name */
    hp = gethostbyname(server_name);
    if(hp == 0) {
        fprintf(stderr, "%s: unknown host\n", server_name);
        exit(2);
    }

    /* construct name of socket to send to */
    bcopy((void *)hp->h_addr, (void *)&sin_addr.sin_addr, hp->h_length);
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port = htons(atoi(port)); /* fixed by adding htons() */

    /* establish connection with server */
    if(CONNECT(sock, (struct sockaddr *)&sin_addr, sizeof(struct sockaddr_in)) < 0) {
        CLOSE(sock);
        perror("error connecting stream socket");
        exit(1);
    }

    /* send file size */
    buf[0]=(file_size>>24)&0xff;
    buf[1]=(file_size>>16)&0xff;
    buf[2]=(file_size>>8)&0xff;
    buf[3]=(file_size)&0xff;
    w_val=SEND(sock,buf,4,0);
    if(w_val < 0) {
        perror("error writing on stream socket");
        exit(1);
    }

    /* send file name */
    bzero(buf,BUF_LEN);
    strcpy((char *)buf,file_name);
    w_val=SEND(sock,buf,20,0);
    if(w_val < 0) {
        perror("error writing on stream socket");
        exit(1);
    }


    /* send BUF_LEN bytes each round */
    file_left=file_size;
    while(file_left){
        /* read from local file */
        r_val=fread(buf,sizeof(unsigned char),BUF_LEN,send_file);
        if(r_val<0){
            perror("error reading file");
            exit(1);
        }
        /* last turn */
        if(r_val>file_left){
            r_val=file_left;
        }
        /* send buffer to server */
        w_val=SEND(sock, buf, BUF_LEN,0);
        if(w_val < 0) {
            perror("error writing on stream socket");
            exit(1);
        }
        /* handle left */
        file_left=file_left-r_val;
    }
    printf("Client sends: %lu bytes\n", file_size);
    CLOSE(sock);
    fclose(send_file);
    return 0;
}
