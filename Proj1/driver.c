/*
CSE 5462 project 1
Nathan Turner 200229714
driver process
Usage: ./driver
*/

#include "stdafx.h"

char TimerPort[8]=TIMER_PORT;

int main()
{
    GlobalInitialize();
    g_message("Run driver Process");
    g_debug("Constructing driver socket ...");
    int DriverSocket;
    DriverSocket=socket(AF_INET, SOCK_DGRAM, 0);
    if(DriverSocket<0){
        perror("error opening driver socket\n");
        exit(1);
    }
    g_info("Constructed driver socket with fd %d",DriverSocket);
    g_debug("Initializing Timer Addr ...");
    struct sockaddr_in TimerAddr;
    TimerAddr.sin_family = AF_INET;
    struct hostent *LocalHostEnt = gethostbyname("localhost");
    if(LocalHostEnt == NULL) {
        perror("localhost: unknown host\n");
        exit(3);
    }
    bcopy((char *)LocalHostEnt ->h_addr, (char *)&TimerAddr.sin_addr, LocalHostEnt ->h_length);
    TimerAddr.sin_port = htons(atoi(TimerPort));
    g_debug("Generating session id ...");
    int Session=rand();
    g_info("Generated session %d",Session);

    g_debug("Sending timer requests ...");
#define StartTimer(SeqNum,Seconds) SendTimerRequest(DriverSocket,&TimerAddr,Session,SeqNum,Seconds)
#define CancelTimer(SeqNum) CancelTimerRequest(DriverSocket,&TimerAddr,Session,SeqNum)
    StartTimer(1,20);
    StartTimer(2,10);
    StartTimer(3,30);
    g_message("Sleeping %d seconds",5);
    sleep(5);
    CancelTimer(2);
    StartTimer(4,20);
    g_message("Sleeping %d seconds",5);
    sleep(5);
    StartTimer(5,18);
    CancelTimer(4);
    CancelTimer(8);

    close(DriverSocket);
    return 0;
}

