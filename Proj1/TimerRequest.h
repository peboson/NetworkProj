#ifndef _TIMER_REQUEST_H_
#define _TIMER_REQUEST_H_

#include <unistd.h>

#define TimerRequestType uint8_t
#define TimerRequestStart 1
#define TimerRequestCancel 2

struct TimerRequestStruct{
    TimerRequestType Type;
    int32_t Session;
    int32_t SeqNum;
    double Seconds;
};

struct TimerExpiredStruct{
    int32_t Session;
    int32_t SeqNum;
};

ssize_t SendTimerRequest(int TimerSocket,struct sockaddr_in * TimerAddr,int Session,int SeqNum,double Seconds);
ssize_t CancelTimerRequest(int TimerSocket,struct sockaddr_in * TimerAddr,int Session,int SeqNum);

#endif
