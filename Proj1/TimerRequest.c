#include "stdafx.h"
#include "TimerRequest.h"
ssize_t SendTimerRequest(int TimerSocket,struct sockaddr_in * TimerAddr,int Session,int SeqNum,double Seconds){
    g_debug("Start Timer Request SeqNum:%d Seconds:%0.2lf ...",SeqNum,Seconds);
    g_debug("Constructing TimerRequest ...");
    struct TimerRequestStruct NowRequest;
    NowRequest.Type=TimerRequestStart;
    NowRequest.Session=Session;
    NowRequest.SeqNum=SeqNum;
    NowRequest.Seconds=Seconds;
    g_debug("Sending start timer request ...");
    ssize_t ret=sendto(TimerSocket,(void *)&NowRequest,sizeof(NowRequest),0,(struct sockaddr *)TimerAddr,sizeof(*TimerAddr));
    g_message("Sent start timer request SeqNum:%d Seconds:%0.2lf",SeqNum,Seconds);
    g_debug("Start Timer Request done.");
    return ret;
}
ssize_t CancelTimerRequest(int TimerSocket,struct sockaddr_in * TimerAddr,int Session,int SeqNum){
    g_debug("Cancel Timer Request SeqNum:%d ...",SeqNum);
    g_debug("Constructing TimerRequest ...");
    struct TimerRequestStruct NowRequest;
    NowRequest.Type=TimerRequestCancel;
    NowRequest.Session=Session;
    NowRequest.SeqNum=SeqNum;
    NowRequest.Seconds=0;
    g_debug("Sending cancel timer request ...");
    ssize_t ret=sendto(TimerSocket,(void *)&NowRequest,sizeof(NowRequest),0,(struct sockaddr *)TimerAddr,sizeof(*TimerAddr));
    g_message("Sent cancel timer request SeqNum:%d",SeqNum);
    g_debug("Cancel Timer Request done.");
    return ret;
}
