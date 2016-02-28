/*
CSE 5462 project 1
Nathan Turner 200229714
timer process
Usage: ./timer
*/

#include "stdafx.h"

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
#include <stdint.h>
#include <sys/time.h>
#define STARTTIMER 1
#define ENDTIMER 0

//node structure for list of packet timers
struct node{
    struct node *next;
    struct node *prev;
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
};
struct packet pack;

int port=8080;

int addNode(struct node **head);
int deleteNode(struct node **head, int seq);
int checkForNode(struct node *head, int seq);
void updateNodes(struct node **head);
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
    timer_addr.sin_family=AF_INET;
    timer_addr.sin_addr.s_addr=INADDR_ANY;
    timer_addr.sin_port=htons(port);
    struct sockaddr_in cli;
    socklen_t len=sizeof(cli);

        //bind tcpd socket
    if(bind(driver,(struct sockaddr *)&timer_addr,sizeof(timer_addr))<0){
        perror("error binding socket");
        exit(1);
    }

    //initialize head of node to null
    struct node *head;
    head=NULL;

    //initialize select values
    fd_set fds;
    struct timeval t;    
    while(1){
        //set fds values
        FD_ZERO(&fds);
        FD_SET(driver,&fds);
        if(head==NULL){
            t.tv_sec=1000;
            t.tv_usec=0;
        }else{
            t.tv_sec=(unsigned)(head->timeDiff);
            t.tv_usec=(unsigned)((head->timeDiff-t.tv_sec)*1000000);
        }
        //call select with timeout looking at local socket
        int rcv=select(driver+1,&fds,NULL,NULL,&t);
        //if pack is in sock then read it
        if(rcv>0){
            recvfrom(driver,(void *)&pack,sizeof(pack),0,(struct sockaddr *)&cli,&len);            
            //if call for start of new timer then add node to list            
            if(pack.start==STARTTIMER){
                if(!addNode(&head))
                    printf("error adding dup node %d\n",pack.seq);
                else
                    printf("added node %d, with timer %.02f\n",pack.seq,pack.time);
            }
            //if call to end timer delete node from list
            if(pack.start==ENDTIMER){
                if(!deleteNode(&head,pack.seq))
                    printf("error on delete seq %d\n",pack.seq);
                else
                    printf("deleted node %d\n",pack.seq);
            }

        }
        //update timers on nodes
        updateNodes(&head);    
        //print current node list
        printNodes(head);
    }
    //close sock
    close(driver);
}


//add node to list of seq# timers given pointer to head of node
int addNode(struct node **head){
    //create new node with given values
    struct node *newNode;
    newNode=(struct node *)malloc(sizeof(struct node));
    newNode->timeDiff=pack.time;
    newNode->port=pack.returnPort;
    newNode->seq=pack.seq;
    newNode->next=NULL;
    newNode->prev=NULL;    
    //temp node pointer to iterate through list
    struct node *temp=*head;

    //if seq# is already in list return
    if(checkForNode(*head, pack.seq)){
        return 0;
    }
    //if list is empty make new node first node in list
    if(*head == NULL){
        *head=newNode;
    }
    //else if new node timer is less than start of list add to front of list
    else if(temp->timeDiff > pack.time){
        newNode->next=temp;
        temp->prev=newNode;
        *head=newNode;
    }
    else if(temp->next==NULL){
        temp->next=newNode;
        newNode->prev=temp;
    }
    //if not first element in list
    else{
        //lower time of node as you iterate through list
        newNode->timeDiff-=temp->timeDiff;
        temp=temp->next;
        int added=0;
        //while in middle of list and new node hasnt been added
        while(temp->next != NULL && added==0){
            //add node to middle of list
            if(newNode->timeDiff <= temp->timeDiff){
                newNode->next=temp;
                newNode->prev=temp->prev;
                temp->prev->next=newNode;
                temp->prev=newNode;
                added=1;
                break;
            }
            newNode->timeDiff-=temp->timeDiff;                    
            temp=temp->next;
        }
        //place new node at end of list
        if(temp->next == NULL && temp->timeDiff < newNode->timeDiff && added==0){
            newNode->prev=temp;
            newNode->timeDiff-=temp->timeDiff;
            temp->next=newNode;
            added=1;
        }
        //place new node directly before last node in list
        else if(temp->next == NULL && temp->timeDiff >= newNode->timeDiff && added==0){
            newNode->next=temp;
            newNode->prev=temp->prev;
            temp->prev->next=newNode;
            temp->prev=newNode;
            added=1;
        }
    }

    //if theres another node after newNode get time difference
    temp=newNode->next;
    if(temp!=NULL)
        temp->timeDiff-=newNode->timeDiff;
    return 1;
}

//delete node from list with given seq# and pointer to head of node
int deleteNode(struct node **head, int seq){
    int del=0;
    struct node *temp=*head;

    //make sure list isnt empty
    if(temp!=NULL){
        //if first node is node to delete
        if(temp->seq == seq){
            //remove node and update next nodes timer
            *head=temp->next;
            if(temp->next!=NULL){
                temp->next->prev=NULL;
                temp->next->timeDiff+=temp->timeDiff;
            }
            free(temp);
            del=1;
        }
        else{
            //look through middle nodes
            temp=temp->next;
            if(temp!=NULL){
                while(temp->next!=NULL){
                    //remove node and update next nodes timer
                    if(temp->seq==seq){
                        if(temp->next!=NULL)
                            temp->next->timeDiff+=temp->timeDiff;
                        temp->prev->next=temp->next;
                        temp->next->prev=temp->prev;
                        del=1;
                        free(temp);
                        break;
                    }

                    temp=temp->next;
                }
                //if last node
                if(del==0 && temp->seq==seq){        
                    temp->prev->next=temp->next;
                    free(temp);
                    del=1;                
                }    
            }
        }

    }
    return del;
}

//find node with given seq number
int checkForNode(struct node *head, int seq){
    while(head != NULL){
        if(head->seq == seq){
            return 1;
        }
        head=head->next;
    }
    return 0;
}

//update node timers
void updateNodes(struct node **head){
    struct node *temp=*head;
    //get time difference
    static struct timeval lasttime={.tv_usec=0};
    if(lasttime.tv_usec==0){
        gettimeofday(&lasttime,NULL);
        //lasttime=time(NULL);
    }
    struct timeval nowtime;
    gettimeofday(&nowtime,NULL);
    //printf("%lu %lu\n", nowtime.tv_sec, nowtime.tv_usec);
    double ElapsedTime=(nowtime.tv_sec*1.0-lasttime.tv_sec*1.0)+(nowtime.tv_usec*1.0-lasttime.tv_usec*1.0)/1000000;
    //printf("%lf\n" ,ElapsedTime);
    lasttime=nowtime;

    //update first node timer and remove if <=0
    if(temp != NULL){
        temp->timeDiff-=ElapsedTime;
        if(temp->timeDiff <= 0){
            int seq=temp->seq;
            //call back to driver here
            if(deleteNode(head,seq))
                printf("node %d expired\n",seq);
        }
        temp=temp->next;
    }
}

//print node list
void printNodes(struct node *head){
    struct node *temp=head;
    if(temp != NULL){
        printf("Nodes: ");
        while(temp != NULL){
            printf("seq %d, time %.2f \t",temp->seq,temp->timeDiff);
            temp=temp->next;
        }
        printf("\n");
    }
}
