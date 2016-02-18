/*
CSE 5462 project 1
Nathan Turner 200229714
timer process
Usage: ./timer
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

//node structure for list of packet timers
struct node{
	struct node *next;
	struct node *prev;
	double time;
	double timeDiff;
	int port;
	int seq;
};

//packet with timer info 
struct packet{
        int returnPort;
	int seq;
	double time; 
	int start; //start or cancel timer
}pack;

int port=3000;

int addNode(struct node **head);
int deleteNode(struct node **head, int seq);
int checkForNode(struct node *head, int seq);
void updateNodes(struct node **head, struct timeval time);
void printNodes(struct node *head);

int main()
{
        int driver;

        //local driver socket
        driver=socket(AF_INET, SOCK_DGRAM, 0); //create socket
        if(driver<0){
                printf("error opening socket\n");
                exit(1);
        }

	//local socket information
        struct sockaddr_in timer_addr;
        timer_addr.sin_family=htons(AF_INET);
        timer_addr.sin_addr.s_addr=inet_addr("0");
//        timer_addr.sin_addr.s_addr=htonl(INADDR_ANY);
        timer_addr.sin_port=htons(port);
        //bind tcpd socket
        if(bind(driver,(struct sockaddr *)&timer_addr,sizeof(timer_addr))<0){
                printf("error on local socket binding\n");
                exit(1);
        }

	//initialize head of node to null
	struct node *head;
	head=NULL;

	//initialize select values
	fd_set fds;
	struct timeval t;	
	t.tv_sec=10;
	t.tv_usec=500;
	int start=0;
	struct timeval startT;
	while(1){
		//set fds values
		FD_ZERO(&fds);
		FD_SET(driver,&fds);
		struct sockaddr_in cli;
		int len=sizeof(cli);

                int rcv=select(driver+1,&fds,NULL,NULL,&t);
		if(rcv>0){
			recvfrom(driver,(void *)&pack,sizeof(pack),0,(struct sockaddr *)&cli,&len);
			start++;
			if(pack.start==1){
				if(!addNode(&head))
					printf("error adding node\n");
				printf("added node %d,%.02f\n",pack.time,pack.seq);
			}
//			if(pack.start==0){
//				if(!deleteNode(&head,pack.seq))
//					printf("error on delete\n");
//				printf("deleted node %d\n",pack.seq);
//			}
		}
//		if(star==1)
			
//			updateNodes(&head,t);

		t.tv_sec=10;
		t.tv_usec=500;
		if(head != NULL){
			t.tv_sec=(int)head->time;
			double usec=head->time - t.tv_sec;
			t.tv_usec=usec*1000;
		}

		printNodes(head);
	}
	close(driver);
}


int addNode(struct node **head){
	int added=0;
	struct node *newNode;
	newNode=(struct node *)malloc(sizeof(struct node));
	newNode->time=pack.time;
	newNode->timeDiff=pack.time;
	newNode->port=pack.returnPort;
	newNode->seq=pack.seq;
	newNode->next=NULL;
	newNode->prev=NULL;	
	struct node *temp=*head;

	if(checkForNode(*head, pack.seq)){
		printf("error: adding dup seq#\n");
	}
	else{
		if(*head == NULL){
			*head=newNode;
			added=1;
		}
		else if(temp->time > pack.time){
			newNode->next=temp;
			temp->prev=newNode;
			*head=newNode;
			added=1;
		}
	 	else{
			temp=temp->next;
			while(temp->next != NULL && added==0){
				if(pack.time <= temp->time){
					newNode->next=temp;
					newNode->prev=temp->prev;
					temp->prev->next=newNode;
					temp->prev=newNode;
					added=1;
					break;
				}					
				temp=temp->next;
			}
			if(temp->next == NULL && temp->time < pack.time && added==0){
				newNode->prev=temp;
				temp->next=newNode;
				added=1;
			}
			else if(temp->next == NULL && temp->time >= pack.time && added==0){
				newNode->next=temp;
				newNode->prev=temp->prev;
				temp->prev->next=newNode;
				temp->prev=newNode;
				added=1;
			}
		newNode->timeDiff-=newNode->prev->time;
		}
	}

	temp=newNode->next;
	if(temp!=NULL)
		temp->timeDiff-=newNode->timeDiff;
	return added;
}

int deleteNode(struct node **head, int seq){
	int del=0;
	struct node *temp=*head;
	if(temp!=NULL){
		if(temp->seq == seq){
			*head=temp->next;
			temp->next->prev=NULL;
			if(temp->next!=NULL)
				temp->next->timeDiff-=temp->timeDiff;
			free(temp);
			del=1;
		}
		else{
			temp=temp->next;
			while(temp->next!=NULL){
				if(temp->seq==seq){
					if(temp->next!=NULL)
						temp->next->timeDiff-=temp->timeDiff;
					temp->prev->next=temp->next;
					temp->next->prev=temp->prev;
					free(temp);
				}
			temp=temp->next;
			}
			if(temp->seq==seq && del==0){
				temp->prev->next=temp->next;
				free(temp);
				del=1;
			}
		}
	}
return del;
}

int checkForNode(struct node *head, int seq){
	int flag=0;
	while(head != NULL){
		if(head->seq == seq){
			flag==1;
			break;
		}
		else
			head=head->next;
	}
	return flag;
}

void updateNodes(struct node **head, struct timeval time){
	struct node *temp=*head;
	double sec=time.tv_sec+(time.tv_sec/1000.);
	printf("time %.2f\n",sec);
	while(temp != NULL){
		temp->timeDiff-=sec;
		if(temp->timeDiff <= 0){
			int seq=temp->seq;
			if(deleteNode(head,seq))
				printf("node %d expired\n",seq);
		}
		temp=temp->next;
	}
}

void printNodes(struct node *head){
printf("Nodes: ");
if(head==NULL)
printf("null head\n");
	while(head != NULL){
		printf("seq %d, time %.2f     ",head->seq,head->timeDiff);
		head=head->next;
	}
	printf("\n");
}
