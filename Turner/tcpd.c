/*
CSE 5462 project 1
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
#include <linux/tcp.h>

//packet with sender enum and  message with length of message attached
struct packet{
	int sender; //1=ftpc, 2=ftps
	int len;
	char mesg[1000]; //1KB message
}pack;

//struct to send data to checksum
struct checksum{
	struct tcphdr head;
	char body[1000];
}chksum;


//global port number
int tcpd_port=8000;
int tcpd_remote_port=8010;
int troll_port=4000;
char *tcpds_ip="164.107.113.10";
int tot=0,tot2=0;

//uint16_t
unsigned short crc16(void* vdata,size_t length) {
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;

    // Initialise the accumulator.
    uint16_t acc=0xffff;
 
    int i;
    // Handle complete 16-bit blocks.
    for (i=0;i+1<length;i+=2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Handle any partial block at the end of the data.
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Return the checksum in network byte order.
    return htons(~acc);
}

unsigned short tcrc16(char *data_p, unsigned short length){
	unsigned char i;
	unsigned int data;
	unsigned int crc=0xffff;

	if(length == 0)
		return (~crc);
	
	do{
		for(i=0, data=(unsigned int)0xff & *data_p++; i<8; i++, data >>= 1){
			if((crc & 0x0001)^(data & 0x0001))
				crc = (crc>>1)^0x8408;
			else
				crc >>= 1;
		}
	}while(--length);

	crc = ~crc;
	data = crc;
	crc = (crc <<8) | (data >> 8 & 0xff);
	
	return (crc);
}

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
		struct tcphdr tcpHeader;
		char body[1000];
	} message;

	while(1){
	 	//read from socket
		int rcv=recvfrom(tcpd,(void *)&pack,sizeof(pack),0,(struct sockaddr *)&cli,&len);
		if(rcv<0){
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
			
			rcv-=2*sizeof(int);
			tot++;
			printf("package %d from client\n",tot);
			memcpy(message.body,pack.mesg,pack.len);

			//set checksum
			chksum.head=message.tcpHeader;
			memcpy(chksum.body,message.body,pack.len);
			message.tcpHeader.check=0;
			message.tcpHeader.check=crc16((char *)&chksum,sizeof(struct tcphdr)+pack.len);

			//send message to troll
			sendto(tcpdt, (char *)&message, sizeof(struct sockaddr_in)+sizeof(struct tcphdr)+pack.len, 0, (struct sockaddr *)&troll, sizeof(troll));
		}
		//if package from server
		else if(pack.sender==2){
	//		printf("request from server\n");
			//recieve from troll
			int read=recvfrom(tcpdr,(char *)&message,sizeof(struct sockaddr_in)+sizeof(struct tcphdr)+pack.len,0,(struct sockaddr *)&tcpdr_addr,&len);
			if(read<0){
				printf("error reading from troll\n");
				exit(1);
			}
			
			//send message to ftps
			read=read-sizeof(struct sockaddr_in)-sizeof(struct tcphdr);
			tot2++;


			//find checksum
			chksum.head=message.tcpHeader;
			memcpy(chksum.body,message.body,read);
unsigned short test=message.tcpHeader.check;
			message.tcpHeader.check=0;
			message.tcpHeader.check=crc16((void *)&chksum,sizeof(struct tcphdr)+read);
printf("package %d from troll\n",tot2);
printf("%hu %hu \n",test,message.tcpHeader.check);
			//check if checksums match
//			if(receivedCheck!=message.tcpHeader.check)
//				printf("checksum error\n");

			sendto(tcpd,message.body,read,0,(struct sockaddr *)&cli,len);
		}

	}

        close(tcpd);
	close(tcpdr);
	close(tcpdt);
	return 0;
}


