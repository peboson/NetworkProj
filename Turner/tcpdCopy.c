/*
CSE 5462 lab3
Nathan Turner 200229714
TCPD process
Usage: ./tcpd
*/

#include <stdio.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

//packet with header of final dest ip and port number with message attached
struct packet{
	int sender; //1=ftpc, 2=ftps
	int len;
	char mesg[1000]; //1KB message
}pack;

//global port number
int tcpd_port=7000;
int tcpd_remote_port=7010;
int troll_port=4000;
char *tcpds_ip="164.107.113.12";
int tot1=0,tot2=0;
int main()
{
	int tcpd,tcpdr,tcpdt;
	char buff[1000];

	//local tcpd socket
	tcpd=socket(AF_INET, SOCK_DGRAM, 0); //create socket
	if(tcpd<0){
		printf("error opening socket\n");
		exit(1);
	}

        tcpdr=socket(AF_INET, SOCK_DGRAM, 0); //create socket
        if(tcpdr<0){
                printf("error opening socket\n");
                exit(1);
        }

        tcpdt=socket(AF_INET, SOCK_DGRAM, 0); //create socket
        if(tcpdr<0){
                printf("error opening socket\n");
                exit(1);
        }

	//local socket information
	struct sockaddr_in tcpd_addr;
	tcpd_addr.sin_family=htons(AF_INET);
	tcpd_addr.sin_addr.s_addr=inet_addr("0");
	tcpd_addr.sin_port=htons(tcpd_port);
        //bind tcpd socket
        if(bind(tcpd,(struct sockaddr *)&tcpd_addr,sizeof(tcpd_addr))<0){
                printf("error on local socket binding\n");
                exit(1);
	}

   	//remote socket information
        struct sockaddr_in tcpdr_addr;
        tcpdr_addr.sin_family=AF_INET;
        tcpdr_addr.sin_addr.s_addr=INADDR_ANY;
        tcpdr_addr.sin_port=htons(tcpd_remote_port);
        //bind tcpd socket
        if(bind(tcpdr,(struct sockaddr *)&tcpdr_addr,sizeof(tcpdr_addr))<0){
                printf("error on remote socket binding\n");
                exit(1);
        }


	struct sockaddr_in cli;
	int len=sizeof(cli);
	struct {
		struct sockaddr_in header;
		char body[1000];
	} message;

	while(1){
	 	//read from socket
		if(recvfrom(tcpd,(void *)&pack,sizeof(pack),0,(struct sockaddr *)&cli,&len)<0){
			printf("error reading");
			exit(1);
		}

		//if package came from client		
		if(pack.sender==1){
		        //dest and troll information
       			struct sockaddr_in dest, troll;
		        
			//fill in server address information
       			message.header.sin_family=htons(AF_INET);
        		message.header.sin_addr.s_addr=inet_addr(tcpds_ip);
        		message.header.sin_port=htons(tcpd_remote_port);

			//fill in troll address information
			troll.sin_family = AF_INET;
        		troll.sin_addr.s_addr = inet_addr("0");
        		troll.sin_port = htons(troll_port);

			printf("package from client\n");
			memcpy(message.body,pack.mesg,pack.len);
tot1+=pack.len;
			printf("sending to troll %d tot: %d\n",pack.len,tot1);
			//send message to troll
			sendto(tcpdt, (char *)&message, sizeof(struct sockaddr_in)+pack.len, 0, (struct sockaddr *)&troll, sizeof(troll));
		}
		//if package from server
		if(pack.sender==2){
			printf("request from server\n");
			//recieve from troll
			int read=recvfrom(tcpdr,(char *)&message,sizeof(struct sockaddr_in)+pack.len,0,(struct sockaddr *)&tcpdr_addr,&len);
			if(read<0){
				printf("error reading from troll\n");
				exit(1);
			}
			printf("package from troll\n");
			//send message to ftps
			read-=sizeof(struct sockaddr_in);
    tot2+=read;           
		        printf("sending to server %d tot:%d\n",read,tot2);
			sendto(tcpd,message.body,read,0,(struct sockaddr *)&cli,len);
		}

	}

        close(tcpd);
	close(tcpdr);
	close(tcpdt);
	return 0;
}


