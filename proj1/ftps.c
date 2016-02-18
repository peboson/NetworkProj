/*
CSE 5462 lab3
Nathan Turner 200229714
TCP server implementation using UDP
Usage: ./ftps <port>
*/

#include "functions.h"

//packet with header of final dest ip and port number with message$
struct packet{
   int sender; //1=ftpc, 2=ftps
   char mesg[1000]; //1KB message
}pack;

//tcpd socket info
struct sockaddr_in tcpd_addr;
int tcpd_port=8000;

int main()
{
	int server_s, clilen, conn;
	char in_buff[1000];

	//initialize sender to ftps
	pack.sender=2;

	struct sockaddr_in cli_addr;
	server_s=SOCKET(AF_INET, SOCK_STREAM, 0); //create socket
	if(server_s<0)
		error("Error opening socket");

	//fill in tcpd socket's address information
	tcpd_addr.sin_family=AF_INET;
	tcpd_addr.sin_addr.s_addr=inet_addr("0");
	tcpd_addr.sin_port=htons(tcpd_port);

	printf("web server waiting on port %d\n",tcpd_port);

	//accept client request
	clilen=sizeof(cli_addr);
	conn=ACCEPT(server_s, (struct sockaddr *) &cli_addr, &clilen);
	if(conn<0)
		error("Error on accept");

	//read in size of file
	int size,convertedSize;
	RECV(server_s,&size,4,MSG_WAITALL);
	convertedSize=ntohl(size);

	//read file title
	char title[20];
	RECV(server_s,title,20,MSG_WAITALL);

	//open file	
	char file[100];
	sprintf(file,"output/%s",title);
	FILE *fp;
	//open output file
	fp  = fopen(file, "wb");
	if (fp == NULL){
		printf("cannot open output file %s\n", file);
		exit(1);
	}

	printf("receiving file of size %d...\n",convertedSize);
	//read in bytes from socket and write to file
	int bytes=0;
	int recvBytes;
	while(bytes<convertedSize){
		bzero(in_buff,sizeof(in_buff));
		recvBytes=RECV(server_s,in_buff,1000,0);
		bytes+=recvBytes;
		fwrite(in_buff,sizeof(char),recvBytes,fp);
	}

	printf("package recieved and sent to %s\n",file);
       	fclose(fp);
	CLOSE(conn);
	return 0;
}


