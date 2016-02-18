/*
CSE 5462 project 1
Nathan Turner 200229714
driver process
Usage: ./driver
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

//packet with timer info
struct packet{
	int returnPort;
        int seq;
        double time;
        int start; //start or cancel timer
}pack;

int port=3000;

void starttimer(double time, int seq);
void canceltimer(int seq);

int timer;
struct sockaddr_in driver_addr;

int main()
{
        //local driver socket
        timer=socket(AF_INET, SOCK_DGRAM, 0); //create socket
        if(timer<0){
                printf("error opening socket\n");
                exit(1);
        }

        //fill in tcpd socket's address information
        driver_addr.sin_family=AF_INET;
        driver_addr.sin_addr.s_addr=inet_addr("0");
        driver_addr.sin_port=htons(port);


	starttimer(20.0,1);
	starttimer(10.0,2);
	starttimer(30.0,3);
	sleep(5);
	canceltimer(2);
	starttimer(20.0,4);
	sleep(5);
	starttimer(18.0,5);
	canceltimer(4);
	canceltimer(8);

	close(timer);
 	return 0;
}



void starttimer(double time, int seq){
	//set packet variables
	pack.returnPort=port;
	pack.seq=seq;
	pack.time=time;
	pack.start=1;
	
	sendto(timer,(void *)&pack,sizeof(pack),0,(struct sockaddr *)&driver_addr,sizeof(driver_addr));
}

void canceltimer(int seq){

}
