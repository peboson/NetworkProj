/*
CSE 5462 lab3
Nathan Turner 200229714
TCP client implementation using UDP
Usage: ./ftps <IP> <port> <file>
*/

#include "functions.h"

//packet with header of final dest ip and port number with message$

//tcpd socket info
struct sockaddr_in tcpd_addr;
int tcpd_port=8000;

int main(int argc, char *argv[])
{
	int client_s,conn;
	char buff[1000];

	//check if correct number of arguments were inputed
	if(argc != 2){
		printf("usage: ./ftps  <file>");
		exit(1);
	}

	client_s=SOCKET(AF_INET, SOCK_STREAM, 0); //create socket
	if(client_s<0)
		error("Error opening socket");

        //connect to server
	struct sockaddr_in serv_addr;
        conn=CONNECT(client_s, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
        if(conn<0)
                error("Error on connection");


	//fill in tcpd socket's address information
   	tcpd_addr.sin_family=AF_INET;
 	tcpd_addr.sin_addr.s_addr=inet_addr("0");
	tcpd_addr.sin_port=htons(tcpd_port);

	int file_size;
	FILE *fp;
	//open input file
	fp  = fopen(argv[1], "rb");
	if (fp == NULL){
		printf("cannot open input file %s\n", argv[1]);
		exit(1);
	}
	//find size of file
        fseek(fp,0,SEEK_END);
	file_size = ftell(fp);
	fseek(fp,0,0);

	//send server size of file
	int size=htonl(file_size);
	SEND(client_s,&size,4,0);

	//send title of file
	sleep(.01);
	SEND(client_s,argv[1],20,0);
	
	printf("sending %d bytes to server\n",file_size);
	//read file and send to server in 1000 byte increments
	int read;
	while(!feof(fp)){
		bzero(buff,sizeof(buff));
		read=fread(buff,sizeof(char),1000,fp);
		usleep(10000);
		SEND(client_s,&buff,read,0);
	}
	printf("sent %d bytes to server\n",file_size);

	fclose(fp);
	CLOSE(conn);
	return 0;
}

