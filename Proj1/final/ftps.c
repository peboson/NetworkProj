/* server.c using TCP */

/* Server for accepting an Internet stream connection on port 1040 */
#include "stdafx.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>




/* server program called with no argument */
int main(int argc, char *argv[])
{
    GlobalInitialize();
    g_message("Run ftps Process");
    char port[8]=SERVER_PUBLIC_PORT; /* default port */
    int sock; /* initial server socket descriptor */
    struct sockaddr_in sin_addr; /* structure for socket name setup */

    /* test command line format */
    if(argc==2){
        /* fetch port number */
        strcpy(port,argv[1]);
    }else if(argc==1){
    }else{
        fprintf(stderr,"Incorrect command line format.\n");
        fprintf(stderr,"USAGE: ftps [local_port]\n");
        exit(1);
    }
    g_info("TCP server waiting for remote connection from clients ...\n");
    InitTcpUdp(SERVER_TCPD_PORT);

    /*initialize socket connection in unix domain*/
    if((sock = SOCKET(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("error openting datagram socket");
        exit(1);
    }

    /* construct name of socket to send to */
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_addr.s_addr = INADDR_ANY;
    sin_addr.sin_port = htons(atoi(port));

    /* bind socket name to socket */
    if(BIND(sock, (struct sockaddr *)&sin_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("error binding stream socket");
        exit(1);
    }

    /* listen for socket connection and set max opened socket connetions to 5 */
    LISTEN(sock, 5);
    g_info("Listen on port %s\n",port);

    while(1){
        int sock_client; /* client socket descriptor */
        struct sockaddr_in client_addr; /* client address */
        socklen_t client_addr_len; /* client address length */
        int r_val; /* return value from read */
        int fw_val; /* return value from fwrite */
        unsigned int file_size,file_left;
        unsigned char buf_in[BUF_LEN];
        char file_name[21];
        FILE *recv_file;

        /* accept a (1) connection in socket msgsocket */ 
        client_addr_len=sizeof(client_addr);
        if((sock_client = ACCEPT(sock, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) { 
            perror("error connecting stream socket");
            exit(1);
        } 

        /* put all zeros in buffer (clear) */
        bzero(buf_in,BUF_LEN);

        /* read file_size from sock_client */
        r_val=RECV(sock_client, buf_in, 4, MSG_WAITALL);
        if(r_val < 0) {
            perror("error reading file size");
            exit(1);
        }
        file_size=(buf_in[0]<<24)|(buf_in[1]<<16)|(buf_in[2]<<8)|(buf_in[3]);
        g_info("file size: %u bytes\n", file_size);

        /* read file_name from sock_client */
        r_val=RECV(sock_client, buf_in, 20, MSG_WAITALL);
        if(r_val < 0) {
            perror("error reading file name");
            exit(1);
        }
        buf_in[r_val]=0;
        strcpy(file_name,(const char *)buf_in);
        g_info("file name: %s\n", file_name);

        /* get local file descriptor */
        recv_file=fopen(file_name,"wb");

        /* read BUF_LEN bytes from socket every time */
        file_left=file_size;
        while(file_left){
            if(file_left<BUF_LEN){
                r_val=RECV(sock_client, buf_in, file_left, 0);
            }else{
                r_val=RECV(sock_client, buf_in, BUF_LEN, 0);
            }
            if(r_val<0){
                perror("error reading on stream socket");
                break;
            }
            /* handle last cycle */
            if(r_val>file_left){
                r_val=file_left;
            }
            /* write into file */
            fw_val=fwrite(buf_in,sizeof(unsigned char),r_val,recv_file);
            if(fw_val<0){
                perror("error writing on local file");
                break;
            }
            /* update left bytes */
            file_left=file_left-r_val;
            g_debug("File receives %u",file_size-file_left);
        }
        g_info("Server receives: %u bytes\n", file_size);

        /* close client connection and file */
        CLOSE(sock_client);
        fclose(recv_file);
    }

    /* close all connections and remove socket file */
    CLOSE(sock);
    return 0;
}
