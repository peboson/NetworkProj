#include "functions.h"

//packet with header of final dest ip and port number with message attached
struct packet{
        int sender; //1=ftpc, 2=ftps
	int len;
        char mesg[1000]; //1KB message
}pack;

//tcpd socket info
struct sockaddr_in tcpd_addr;


//make UDP socket from TCP command
int SOCKET(int family, int type, int protocol){
   return socket(family, SOCK_DGRAM, protocol);
}

//send msg to tcpd
int SEND(int sockfd, const void *msg, int len, int flags){
   //send packet to tcpd
   pack.sender=1;
   pack.len=len;
   memcpy(pack.mesg,msg,len);
   return sendto(sockfd,(void *)&pack, 2*sizeof(int)+len, flags, (struct sockaddr *)&tcpd_addr, sizeof(tcpd_addr));
}

//receive buf from tcpd
int RECV(int sockfd, void *buf, int len, int flags){
   //send request for packet
   int socklength=sizeof(tcpd_addr);
   pack.sender=2;
   pack.len=len;
   sendto(sockfd, (void *)&pack, sizeof(pack), 0, (struct sockaddr *)&tcpd_addr, socklength);
   //receive bytes from tcpd through troll
   return recvfrom(sockfd, buf, len, flags, (struct sockaddr *)&tcpd_addr, &socklength); 
}

//NULL function for lab3
int CONNECT(int sockfd, struct sockaddr *serv_addr, int addrlen){
   return 1;
}

//NULL function for lab3
int ACCEPT(int sockfd, void *peer, int *addrlen){
   return 1;
}

//close socket
int CLOSE (int sock){
   return close(sock);
}

