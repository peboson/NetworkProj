/*
CSE 5462 project 1
Nathan Turner 200229714
timer process
Usage: ./timer
*/

#include "stdafx.h"

struct TimerNodeStruct{
    int Session;
    int SeqNum;
    double RemainSeconds;
    struct sockaddr *RequestAddr;
};

int TimerSocket;
GList *TimerList=NULL;
struct timeval LastSystemTime={.tv_sec=0, .tv_usec=0};
char TimerPort[8]=TIMER_PORT;

ERRCODE InsertRequest(const struct TimerRequestStruct * NowRequest,struct sockaddr *RequestAddr);
ERRCODE CancelRequest(const struct TimerRequestStruct * NowRequest);
ERRCODE UpdateTimerList();
ERRCODE SendCallbacks();
ERRCODE DisplayTimerList();

int main()
{
    GlobalInitialize();
    g_message("Run Timer Process");
    g_debug("Fetching last system time ...");
    gettimeofday(&LastSystemTime,NULL);
    g_debug("Constructing timer socket ...");
    TimerSocket=socket(AF_INET, SOCK_DGRAM, 0);
    if(TimerSocket<0){
        perror("error openning datagram socket");
        exit(1);
    }
    g_info("Constructed timer socket with fd %d",TimerSocket);
    g_debug("Binding timer socket to port %s ...",TimerPort);
    struct sockaddr_in TimerAddr;
    TimerAddr.sin_family = AF_INET;
    TimerAddr.sin_addr.s_addr = INADDR_ANY;
    TimerAddr.sin_port = htons(atoi(TimerPort));
    if(bind(TimerSocket, (struct sockaddr *)&TimerAddr, sizeof(TimerAddr)) < 0) {
        perror("error binding stream socket");
        exit(1);
    }
    g_info("Binded timer socket to port %s",TimerPort);
    g_debug("Entering main loop ...");
    fd_set TimerSocketSet;
    struct timeval SleepTime;
    while(1){
        g_debug("Fill waiting sockets set ...");
        FD_ZERO(&TimerSocketSet);
        FD_SET(TimerSocket,&TimerSocketSet);
        g_debug("Setting waiting time ...");
        if(TimerList==NULL){
            SleepTime.tv_sec=1000;
            SleepTime.tv_usec=0;
        }else{
            GList *TimerFirst=g_list_first(TimerList);
            SleepTime.tv_sec=(unsigned)(((struct TimerNodeStruct *)(((struct TimerNodeStruct *)(TimerFirst->data))))->RemainSeconds);
            SleepTime.tv_usec=(unsigned)(SEC2USEC*(((struct TimerNodeStruct *)(TimerFirst->data))->RemainSeconds-SleepTime.tv_sec));
        }
        g_debug("Waiting incomming requests for %zu .%06zu seconds ...",SleepTime.tv_sec,SleepTime.tv_usec);
	//wait for timer of first node or request received
        int ready=select(TimerSocket+1,&TimerSocketSet,NULL,NULL,&SleepTime);
        g_debug("Request received or time expired");
        g_debug("Updating timer list ...");
        UpdateTimerList();
        if(ready<0){
            perror("error select");
            exit(1);
        }
        g_debug("Checking received request ...");
        if(ready>0){
            if(FD_ISSET(TimerSocket,&TimerSocketSet)){
                g_debug("Request received from TimerSocket");
                struct TimerRequestStruct NowRequest;
                struct sockaddr_in *RequestAddr=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
                socklen_t RequestAddrLength=sizeof(RequestAddr);
                g_debug("Fetching request ...");
                recvfrom(TimerSocket,(void *)&NowRequest,sizeof(NowRequest),MSG_WAITALL,(struct sockaddr *)RequestAddr,&RequestAddrLength);
                g_info("Request fetched Type:%d SeqNum:%d, Seconds:%0.2lf",NowRequest.Type,NowRequest.SeqNum,NowRequest.Seconds);
		//see if request was to add or remove node
                switch(NowRequest.Type){
                    case TimerRequestStart:{
                        InsertRequest(&NowRequest,(struct sockaddr *)RequestAddr);
                        break;
                    }
                    case TimerRequestCancel:{
                        CancelRequest(&NowRequest);
                        break;
                    }
                }
            }
        }
        g_debug("Sending callbacks ...");
        SendCallbacks();
        g_debug("Displaying timer list ...");
        DisplayTimerList();
    }
    g_debug("Closing TimerSocket ...");
    close(TimerSocket);
    return 0;
}

//insert timer node
ERRCODE InsertRequest(const struct TimerRequestStruct * NowRequest, struct sockaddr *RequestAddr){
    g_debug("Call Insert Request ...");
    g_assert(NowRequest);
    g_debug("Constructing node for request %d ...",NowRequest==NULL?-1:NowRequest->SeqNum);
    struct TimerNodeStruct *NowNode=(struct TimerNodeStruct *)malloc(sizeof(struct TimerNodeStruct));
    NowNode->Session=NowRequest->Session;
    NowNode->SeqNum=NowRequest->SeqNum;
    NowNode->RemainSeconds=NowRequest->Seconds;
    NowNode->RequestAddr=RequestAddr;
    g_debug("Checking duplicated SeqNum for %d ...",NowNode->SeqNum);
    GList *l=TimerList;
    while(l!=NULL){
        if((((struct TimerNodeStruct *)(l->data))->Session==NowNode->Session)&&(((struct TimerNodeStruct *)(l->data))->SeqNum==NowNode->SeqNum)){
            g_message("Insert %d failed. Duplicated SeqNum",NowNode->SeqNum);
            return 1;
        }
        l=l->next;
    }
    g_debug("Finding position for SeqNum %d ...",NowNode->SeqNum);
    l=TimerList;
    while(l!=NULL){
        if(NowNode->RemainSeconds<((struct TimerNodeStruct *)(l->data))->RemainSeconds){
            break;
        }
        NowNode->RemainSeconds-=((struct TimerNodeStruct *)(l->data))->RemainSeconds;
        l=l->next;
    }
    g_debug("Inserting SeqNum %d ...",NowNode->SeqNum);
    TimerList=g_list_insert_before(TimerList,l,NowNode);
    g_info("Inserted %d before %d",NowNode->SeqNum,l==NULL?-1:((struct TimerNodeStruct *)(l->data))->SeqNum);
    g_debug("Updating SeqNum %d ...",l==NULL?-1:((struct TimerNodeStruct *)(l->data))->SeqNum);
    if(l!=NULL){
        ((struct TimerNodeStruct *)(l->data))->RemainSeconds-=NowNode->RemainSeconds;
    }
    g_debug("Insert Request done.");
    g_message("Timer request inserted. SeqNum:%d, Seconds:%0.2lf (Session:%d)",NowRequest->SeqNum,NowRequest->Seconds,NowRequest->Session);
    return 0;
}

//cancel timer request
ERRCODE CancelRequest(const struct TimerRequestStruct * NowRequest){
    g_debug("Call Cancel Request ...");
    g_assert(NowRequest);
    g_debug("Finding Existence of SeqNum %d ...",NowRequest->SeqNum);
    GList *l=TimerList;
    while(l!=NULL){
        if((((struct TimerNodeStruct *)(l->data))->Session==NowRequest->Session)&&(((struct TimerNodeStruct *)(l->data))->SeqNum==NowRequest->SeqNum)){
            break;
        }
        l=l->next;
    }
    g_debug("Checking Existence of SeqNum %d ...",NowRequest->SeqNum);
    if(l==NULL){
        g_message("Cancel %d failed. Not found.",NowRequest->SeqNum);
        return 1;
    }
    g_debug("Updating SeqNum %d ...",l->next==NULL?-1:((struct TimerNodeStruct *)(l->next->data))->SeqNum);
    if(l->next!=NULL){
        ((struct TimerNodeStruct *)(l->next->data))->RemainSeconds+=((struct TimerNodeStruct *)(l->data))->RemainSeconds;
    }
    g_debug("Deleting SeqNum %d ...",l==NULL?-1:((struct TimerNodeStruct *)(l->data))->SeqNum);
    TimerList=g_list_delete_link(TimerList,l);
    g_info("Deleted SeqNum %d ...",NowRequest->SeqNum);
    g_debug("Cancel Request done.");
    g_message("Timer request canceled. SeqNum:%d, (Session:%d)",NowRequest->SeqNum,NowRequest->Session);
    return 0;
}

//update list with past time
ERRCODE UpdateTimerList(){
    g_debug("Call Update Timer List ...");
    g_assert(LastSystemTime.tv_sec!=0);
    g_debug("Calculating elapsed time ...");
    struct timeval NowSystemTime;
    gettimeofday(&NowSystemTime,NULL);
    double ElapsedSeconds=(double)(NowSystemTime.tv_sec-LastSystemTime.tv_sec) +USEC2SEC*NowSystemTime.tv_usec-USEC2SEC*LastSystemTime.tv_usec;
    LastSystemTime=NowSystemTime;
    g_info("Elapsed %lf seconds",ElapsedSeconds);
    g_debug("Update first timer node ...");
    if(TimerList!=NULL){
        GList *TimerFirst=g_list_first(TimerList);
        ((struct TimerNodeStruct *)(TimerFirst->data))->RemainSeconds-=ElapsedSeconds;
    }
    g_debug("Transmitting over time ...");
    GList *l=TimerList;
    while(l!=NULL){
        if(((struct TimerNodeStruct *)(l->data))->RemainSeconds>0){
            break;
        }
        if(l->next!=NULL){
            ((struct TimerNodeStruct *)(l->next->data))->RemainSeconds+=((struct TimerNodeStruct *)(l->data))->RemainSeconds;
        }
        ((struct TimerNodeStruct *)(l->data))->RemainSeconds=0;
        g_info("SeqNum %d expired",((struct TimerNodeStruct *)(l->data))->SeqNum);
        l=l->next;
    }
    g_debug("Update Timer List done.");
    return 0;
}

//call back tcpd when timer expires
ERRCODE SendCallbacks(){
    g_debug("Call Send Callbacks ...");
    g_debug("Searching Timer List ...");
    while(TimerList!=NULL){
        GList *TimerFirst=g_list_first(TimerList);
        if(((struct TimerNodeStruct *)(TimerFirst->data))->RemainSeconds>0){
            break;
        }
        g_debug("Sending callback with SeqNum %d",((struct TimerNodeStruct *)(TimerFirst->data))->SeqNum);
        struct TimerExpiredStruct NowExpired;
        NowExpired.Session=((struct TimerNodeStruct *)(TimerFirst->data))->Session;
        NowExpired.SeqNum= ((struct TimerNodeStruct *)(TimerFirst->data))->SeqNum;
        struct sockaddr *RequestAddr=((struct TimerNodeStruct *)(TimerFirst->data))->RequestAddr;
        ssize_t SendRet=sendto(TimerSocket,(void *)&NowExpired,sizeof(NowExpired),0,RequestAddr,sizeof(*RequestAddr));
        if(SendRet<0){
            perror("Error sending");
            exit(1);
        }
        g_message("Sent callback with SeqNum %d (Session:%d)",NowExpired.SeqNum,NowExpired.Session);
        TimerList=g_list_delete_link(TimerList,TimerFirst);
        g_info("Deleted SeqNum %d ...",NowExpired.SeqNum);
    }
    g_debug("Send Callbacks done.");
    return 0;
}

//display list for testing
ERRCODE DisplayTimerList(){
    g_debug("Call Display Timer List ...");
    GList *l=TimerList;
    while(l!=NULL){
        g_info("TimerNode SeqNum: %d, RemainSeconds: %0.2lf (Session:%d)", ((struct TimerNodeStruct *)(l->data))->SeqNum, ((struct TimerNodeStruct *)(l->data))->RemainSeconds, ((struct TimerNodeStruct *)(l->data))->Session);
        l=l->next;
    }
    g_debug("Display Timer List done.");
    return 0;
}
