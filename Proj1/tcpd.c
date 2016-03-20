/* File: tcpd.c
 * Author: Jason Song.988
 * Description: CSE5462 Lab3
 *     The deamon process running behind TCP programs to wrap UDP as TCP
 */

#include "stdafx.h"

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

int sock; /* internal communication socket */
#define TcpdMsgSocket sock
char buf[BUF_LEN];
struct sockaddr_in tcpd_name;
struct sockaddr_in TrollAddr;

socklen_t tcpd_name_len;
char roleport[MAX_PATH];

struct hostent *LocalHostEnt; /* troll server host */

int TimerDriverSocket;
struct sockaddr_in TimerAddr;

struct BindedSocketStruct {
    int SocketFD;
    int Accepting;
    struct sockaddr_in AcceptingAddr;
};

GArray * BindedSockets;

struct AcceptedSocketStruct{
    int SocketFD;
    uint32_t Session;
    uint32_t Base;
    size_t ReceivingLength;
    struct sockaddr_in ReceivingAddr;
    GList *Packages;
    char RecvBuffer[BUF_LEN];
    size_t RecvBufferSize;
};
GHashTable *AcceptedSockets;
GList *WaitingAcceptSocketsList;

struct ConnectedSocketStruct{
    int SocketFD;
    uint32_t Session;
    uint32_t Base;
    uint32_t MinSeq;
    struct sockaddr_in ServerAddr;
    GList *Packages;
    GList *PendingPackages;
};
GHashTable *ConnectedSockets;

ERRCODE RecvData(struct BindedSocketStruct *NowBindedSocket);
ERRCODE RecvAck(struct ConnectedSocketStruct *NowConnectedSocket);
ERRCODE RecvTimer();
ERRCODE SolveAcceptCallback();
ERRCODE SolveRecvCallback();
ERRCODE SolveSendCallback();
ERRCODE PrintTcpdStatus();
ERRCODE tcpd_SOCKET(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength);
ERRCODE tcpd_BIND(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength);
ERRCODE tcpd_ACCEPT(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength);
ERRCODE tcpd_RECV(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength);
ERRCODE tcpd_CONNECT(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength);
ERRCODE tcpd_SEND(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength);

int InBufferPackages=0;

int main(int argc, char *argv[]) /* server program called with no argument */
{
    GlobalInitialize();
    BindedSockets=g_array_new(FALSE,FALSE,sizeof(struct BindedSocketStruct *));
    AcceptedSockets=g_hash_table_new(NULL,NULL);
    ConnectedSockets=g_hash_table_new(NULL,NULL);
    /* fetch console arguments */
    if(argc==2){
        strcpy(roleport,argv[1]);
    }else{
        g_warning("USAGE: ftpd server|client|port");
        exit(1);
    }

    /* create internal communication socket */
    sock = socket(AF_INET, SOCK_DGRAM,0);
    if(sock < 0) {
        perror("opening datagram socket");
        exit(2);
    }

    /* create name with parameters and bind name to socket */
    tcpd_name.sin_family = AF_INET;
    if(strcmp(roleport,"server")==0){
        tcpd_name.sin_port = htons(atoi(SERVER_TCPD_PORT));
    }else if(strcmp(roleport,"client")==0){
        tcpd_name.sin_port = htons(atoi(CLIENT_TCPD_PORT));
    }else{
        tcpd_name.sin_port = htons(atoi(roleport));
    }
    tcpd_name.sin_addr.s_addr = INADDR_ANY;

    /* bind the socket with name */
    if(bind(sock, (struct sockaddr *)&tcpd_name, sizeof(tcpd_name)) < 0) {
        perror("error binding socket");
        exit(2);
    }
    tcpd_name_len=sizeof(tcpd_name);
    /* Find assigned port value and print it for client to use */
    if(getsockname(sock, (struct sockaddr *)&tcpd_name, &tcpd_name_len) < 0){
        perror("error getting sock name");
        exit(3);
    }

    g_debug("Fetching localhost entity");
    LocalHostEnt = gethostbyname("localhost");
    if(LocalHostEnt == NULL) {
        perror("localhost: unknown host");
        exit(3);
    }

#ifdef __WITH_TROLL
    /* construct name of socket to send to */
    TrollAddr.sin_family = AF_INET;
    bcopy((void *)LocalHostEnt->h_addr, (void *)&TrollAddr.sin_addr, LocalHostEnt->h_length);
    TrollAddr.sin_port = htons(atoi(TROLL_PORT));
#endif

    g_info("Server waiting on port # %d", ntohs(tcpd_name.sin_port));

    g_debug("Setting up timer ...");
    g_debug("Constructing driver socket ...");
    TimerDriverSocket=socket(AF_INET, SOCK_DGRAM, 0);
    if(TimerDriverSocket<0){
        perror("error opening driver socket");
        exit(1);
    }
    g_info("Constructed driver socket with fd %d",TimerDriverSocket);
    g_debug("Initializing Timer Addr ...");
    char TimerPort[8]=TIMER_PORT;
    TimerAddr.sin_family = AF_INET;
    bcopy((char *)LocalHostEnt ->h_addr, (char *)&TimerAddr.sin_addr, LocalHostEnt ->h_length);
    TimerAddr.sin_port = htons(atoi(TimerPort));

    /* main loop, drived by upper local process */
    fd_set WaitingSocketSet;
    while(1){
        g_debug("#################### NEXT ROUND ####################");
        g_debug("Filling waiting sockets set ...");
        FD_ZERO(&WaitingSocketSet);
        FD_SET(TimerDriverSocket,&WaitingSocketSet);
        int MaxSocket=TimerDriverSocket;
        if(InBufferPackages<MAX_BUFFER_PACKAGES){
            FD_SET(TcpdMsgSocket,&WaitingSocketSet);
            MaxSocket=TcpdMsgSocket>MaxSocket?TcpdMsgSocket:MaxSocket;
        }
        uint i;
        for(i=0;i<BindedSockets->len;i++){
            struct BindedSocketStruct *TmpBindedSocket=g_array_index(BindedSockets,struct BindedSocketStruct *,i);
            FD_SET(TmpBindedSocket->SocketFD,&WaitingSocketSet);
            MaxSocket=TmpBindedSocket->SocketFD>MaxSocket?TmpBindedSocket->SocketFD:MaxSocket;
        }
        GHashTableIter ConnectedSocketIter;
        int ConnectedSocketSocketFD;
        struct ConnectedSocketStruct *TmpConnectedSocket;
        g_hash_table_iter_init (&ConnectedSocketIter, ConnectedSockets);
        while (g_hash_table_iter_next (&ConnectedSocketIter,(void *)&ConnectedSocketSocketFD, (void *)&TmpConnectedSocket)) {
            FD_SET(TmpConnectedSocket->SocketFD,&WaitingSocketSet);
            MaxSocket=TmpConnectedSocket->SocketFD>MaxSocket?TmpConnectedSocket->SocketFD:MaxSocket;
            g_info("ConnectedSocket %d added to waiting sockets",TmpConnectedSocket->SocketFD);
        }
        g_debug("Setting wait time ...");
        struct timeval SleepTime;
        SleepTime.tv_sec=1000;
        SleepTime.tv_usec=0;
        g_debug("Select sockets ...");
        int ready=select(MaxSocket+1,&WaitingSocketSet,NULL,NULL,&SleepTime);
        if(ready>0){
            g_debug("Testing active socket ...");
            if(FD_ISSET(TcpdMsgSocket,&WaitingSocketSet)){
                g_debug("Receiving Tcpd Message ...");
                struct sockaddr_in FrontAddr;
                socklen_t FrontAddrLength;
                /* fetch the function that upper local process calls */
                char tcpd_msg[TCPD_MSG_LEN];
                FrontAddrLength= sizeof(FrontAddr);
                if(recvfrom(sock, tcpd_msg, TCPD_MSG_LEN, MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
                    perror("error receiving"); 
                    exit(4);
                }
                g_debug("Server receives: %s", tcpd_msg);
                /* switch by the function */
                if(strcmp(tcpd_msg,"SOCKET")==0){
                    tcpd_SOCKET(FrontAddr,FrontAddrLength);
                }else if(strcmp(tcpd_msg,"BIND")==0){
                    tcpd_BIND(FrontAddr,FrontAddrLength);
                }else if(strcmp(tcpd_msg,"ACCEPT")==0){
                    tcpd_ACCEPT(FrontAddr,FrontAddrLength);
                }else if(strcmp(tcpd_msg,"RECV")==0){
                    tcpd_RECV(FrontAddr,FrontAddrLength);
                }else if(strcmp(tcpd_msg,"CONNECT")==0){
                    tcpd_CONNECT(FrontAddr,FrontAddrLength);
                }else if(strcmp(tcpd_msg,"SEND")==0){
                    tcpd_SEND(FrontAddr,FrontAddrLength);
                }
            }
            uint i;
            for(i=0;i<BindedSockets->len;i++){
                struct BindedSocketStruct *TmpBindedSocket=g_array_index(BindedSockets,struct BindedSocketStruct *,i);
                if(FD_ISSET(TmpBindedSocket->SocketFD,&WaitingSocketSet)){
                    RecvData(TmpBindedSocket);
                }
            }
            GHashTableIter ConnectedSocketIter;
            int ConnectedSocketSocketFD;
            struct ConnectedSocketStruct *TmpConnectedSocket;
            g_hash_table_iter_init (&ConnectedSocketIter, ConnectedSockets);
            while (g_hash_table_iter_next (&ConnectedSocketIter,(void *)&ConnectedSocketSocketFD, (void *)&TmpConnectedSocket)) {
                if(FD_ISSET(TmpConnectedSocket->SocketFD,&WaitingSocketSet)){
                    g_info("Received Ack");
                    RecvAck(TmpConnectedSocket);
                }
            }
            if(FD_ISSET(TimerDriverSocket,&WaitingSocketSet)){
                g_debug("Receiving timer expired request");
                RecvTimer();
            }
        }
        SolveAcceptCallback();
        SolveRecvCallback();
        SolveSendCallback();
        PrintTcpdStatus();
    }

    /* server terminates connection, closes socket, and exits */
    close(sock);
    exit(0);
}
ERRCODE RecvData(struct BindedSocketStruct *NowBindedSocket){
    g_assert(NowBindedSocket);
    g_debug("Receiving data sent to socket %d ...",NowBindedSocket->SocketFD);
    struct TrollMessageStruct *NowTrollMessage=(struct TrollMessageStruct *)malloc(sizeof(struct TrollMessageStruct));
#ifdef __WITH_TROLL
    struct sockaddr_in RemoteTrollAddr;
    socklen_t RemoteTrollAddrLength=sizeof(RemoteTrollAddr);
    g_debug("Receiving data from troll ...");
    ssize_t RecvRet=recvfrom(NowBindedSocket->SocketFD, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&RemoteTrollAddr, &RemoteTrollAddrLength);
#else
    struct sockaddr_in ClientAddr;
    socklen_t ClientAddrLength=sizeof(ClientAddr);
    g_debug("Receiving data directly from client ...");
    ssize_t RecvRet=recvfrom(NowBindedSocket->SocketFD, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&ClientAddr, &ClientAddrLength);
    NowTrollMessage->header=ClientAddr;
#endif
    if(RecvRet< 0) {
        perror("error receiving Data");
        exit(5);
    }   
    // g_debug("Checking package length is garbled or not ...");
    // if(NowTrollMessage->len>sizeof(NowTrollMessage->body)){
    //     g_info("[GARBLED] Length:%zu",NowTrollMessage->len);
    //     return -1;
    // }
    g_debug("Checking package body is garbled or not ...");
    uint32_t CheckSum=CHECKSUM_ALGORITHM(0,((char *)(NowTrollMessage))+CheckSumOffset,sizeof(*NowTrollMessage)-CheckSumOffset);
    if(CheckSum!=NowTrollMessage->CheckSum){
        g_info("[GARBLED] CheckSum: %u, Origin CheckSum: %u", CheckSum, NowTrollMessage->CheckSum);
        return -2;
    }
    g_debug("Finding Accepted socket with session %u",NowTrollMessage->Session);
    struct AcceptedSocketStruct *NowAcceptedSocket=g_hash_table_lookup(AcceptedSockets,GUINT_TO_POINTER(NowTrollMessage->Session));
    if(NowAcceptedSocket==NULL){
        GList *l;
        for(l=WaitingAcceptSocketsList;l!=NULL;l=l->next){
            if(((struct AcceptedSocketStruct *)(l->data))->Session==NowTrollMessage->Session){
                NowAcceptedSocket=(struct AcceptedSocketStruct *)(l->data);
                break;
            }
        }
    }
    g_debug("Constructing Accepted Socket if not found in AcceptedSockets or WaitingAcceptSocketsList ...");
    if(NowAcceptedSocket==NULL){
        NowAcceptedSocket=(struct AcceptedSocketStruct *)malloc(sizeof(struct AcceptedSocketStruct ));
        NowAcceptedSocket->SocketFD=NowBindedSocket->SocketFD;
        NowAcceptedSocket->Session=NowTrollMessage->Session;
        NowAcceptedSocket->Base=0;
        NowAcceptedSocket->Packages=NULL;
        NowAcceptedSocket->ReceivingLength=0;
        NowAcceptedSocket->RecvBufferSize=0;
        WaitingAcceptSocketsList=g_list_append(WaitingAcceptSocketsList,NowAcceptedSocket);
    }

    g_debug("Inserting received package into packages buffer ...");
    int RequireValid=TRUE;
    if(NowAcceptedSocket->Base>NowTrollMessage->SeqNum){
        g_info("[OUTDATED] Received Outdated package");
        RequireValid=FALSE;
    }
    GList *l;
    for(l=NowAcceptedSocket->Packages;l!=NULL;l=l->next){
        struct TrollMessageStruct *TmpTrollMessage=(struct TrollMessageStruct *)(l->data);
        if(TmpTrollMessage->SeqNum==NowTrollMessage->SeqNum){
            g_info("[DUPLICATED] Received duplicated package");
            RequireValid=FALSE;
            break;
        }
        if(TmpTrollMessage->SeqNum>NowTrollMessage->SeqNum){
            break;
        }
    }
    if(RequireValid){
        NowAcceptedSocket->Packages=g_list_insert_before(NowAcceptedSocket->Packages,l,NowTrollMessage);
    }
    g_debug("Inserted received package");
    g_debug("Constructing Ack package ...");
    struct TrollAckStruct *NowTrollAck=(struct TrollAckStruct *)malloc(sizeof(struct TrollAckStruct));
    NowTrollAck->header=NowTrollMessage->header;
    NowTrollAck->header.sin_family=htons(AF_INET);
    NowTrollAck->Session=NowTrollMessage->Session;
    NowTrollAck->SeqNum=NowTrollMessage->SeqNum;
    NowTrollAck->CheckSum=CHECKSUM_ALGORITHM(0,((char *)(NowTrollAck))+CheckSumOffset,sizeof(*NowTrollAck)-CheckSumOffset);
    g_info("Sending Ack Addr:%s, Port:%u, SeqNum:%u (Session:%u)",inet_ntoa(NowTrollAck->header.sin_addr),ntohs(NowTrollAck->header.sin_port),NowTrollAck->SeqNum,NowTrollAck->Session);
    g_debug("Sending ACK back ...");
    ssize_t AckRet=sendto(NowBindedSocket->SocketFD, (void *)NowTrollAck, sizeof(*NowTrollAck), MSG_WAITALL, (struct sockaddr *)&TrollAddr, sizeof(TrollAddr));
    if(AckRet < 0) {
        perror("error sending Ack to troll");
        exit(5);
    }
    g_debug("Recv Data done.");
    return 0;
}

ERRCODE RecvAck(struct ConnectedSocketStruct *NowConnectedSocket){
    g_assert(NowConnectedSocket);
    g_debug("Receiving Ack sent to socket %d ...",NowConnectedSocket->SocketFD);
    struct TrollAckStruct *NowTrollAck=(struct TrollAckStruct *)malloc(sizeof(struct TrollAckStruct ));
#ifdef __WITH_TROLL
    struct sockaddr_in RemoteTrollAddr;
    socklen_t RemoteTrollAddrLength=sizeof(RemoteTrollAddr);
    g_debug("Receiving Ack from troll ...");
    ssize_t RecvRet=recvfrom(NowConnectedSocket->SocketFD, (void *)NowTrollAck, sizeof(*NowTrollAck), MSG_WAITALL, (struct sockaddr *)&RemoteTrollAddr, &RemoteTrollAddrLength);
#else
    struct sockaddr_in ClientAddr;
    socklen_t ClientAddrLength=sizeof(ClientAddr);
    g_debug("Receiving Ack directly from client ...");
    ssize_t RecvRet=recvfrom(NowConnectedSocket->SocketFD, (void *)NowTrollAck, sizeof(*NowTrollAck), MSG_WAITALL, (struct sockaddr *)&ClientAddr, &ClientAddrLength);
    NowTrollAck->header=ClientAddr;
#endif
    if(RecvRet< 0) {
        perror("error receiving Ack");
        exit(5);
    }   
    g_debug("Checking package body is garbled or not ...");
    uint32_t CheckSum=CHECKSUM_ALGORITHM(0,((char *)(NowTrollAck))+CheckSumOffset,sizeof(*NowTrollAck)-CheckSumOffset);
    g_info("Received Ack SeqNum:%u (Session:%u)",NowTrollAck->SeqNum,NowTrollAck->Session);
    if(CheckSum!=NowTrollAck->CheckSum){
        g_info("[GARBLED] CheckSum: %u, Origin CheckSum: %u", CheckSum, NowTrollAck->CheckSum);
        return -2;
    }
    g_debug("Removing Ack package with SeqNum %u (Session %u) ...",NowTrollAck->SeqNum,NowTrollAck->Session);
    GList *l;
    for(l=NowConnectedSocket->Packages;l!=NULL;l=l->next){
        struct TrollMessageStruct *TmpTrollMessage=(struct TrollMessageStruct *)(l->data);
        if(TmpTrollMessage->Session==NowTrollAck->Session&&TmpTrollMessage->SeqNum==NowTrollAck->SeqNum){
            NowConnectedSocket->Packages=g_list_remove_link(NowConnectedSocket->Packages,l);
            InBufferPackages--;
            free(TmpTrollMessage);
            g_info("Package SeqNum %u removed. (Session %u)",TmpTrollMessage->SeqNum,TmpTrollMessage->Session);
            break;
        }
    }
    g_debug("Sending cancel timer request with SeqNum %u (Session %u) ...",NowTrollAck->SeqNum,NowTrollAck->Session);
    CancelTimerRequest(TimerDriverSocket,&TimerAddr,NowTrollAck->Session,NowTrollAck->SeqNum);
    free(NowTrollAck);
    return 0;
}

ERRCODE RecvTimer(){
    g_debug("Receiving Timer Expired request ...");
    struct TimerExpiredStruct *NowExpired=(struct TimerExpiredStruct *)malloc(sizeof(struct TimerExpiredStruct ));
    struct sockaddr_in RemoteTimerAddr;
    socklen_t RemoteTimerAddrLength=sizeof(RemoteTimerAddr);
    ssize_t RecvRet=recvfrom(TimerDriverSocket, (void *)NowExpired, sizeof(*NowExpired), MSG_WAITALL, (struct sockaddr *)&RemoteTimerAddr, &RemoteTimerAddrLength);
    if(RecvRet< 0) {
        perror("error receiving Timer Expired");
        exit(5);
    }   
    g_debug("Finding connected socket for timer expired ...");
    struct ConnectedSocketStruct * NowConnectedSocket=NULL;
    GHashTableIter ConnectedSocketIter;
    int ConnectedSocketSocketFD;
    struct ConnectedSocketStruct *TmpConnectedSocket;
    g_hash_table_iter_init (&ConnectedSocketIter, ConnectedSockets);
    while (g_hash_table_iter_next (&ConnectedSocketIter, (void *)&ConnectedSocketSocketFD, (void *)&TmpConnectedSocket))
    {
        if(TmpConnectedSocket->Session==NowExpired->Session){
            NowConnectedSocket=TmpConnectedSocket;
            break;
        }
    }
    if(NowConnectedSocket==NULL){
        perror("socket not found");
        exit(5);
    }
    g_debug("Moving expired package to pending sequence ...");
    GList *l;
    for(l=NowConnectedSocket->Packages;l!=NULL;l=l->next){
        struct TrollMessageStruct *TmpTrollMessage=(struct TrollMessageStruct *)(l->data);
        if(TmpTrollMessage->Session==NowExpired->Session&&TmpTrollMessage->SeqNum==NowExpired->SeqNum){
            NowConnectedSocket->Packages=g_list_remove_link(NowConnectedSocket->Packages,l);
            NowConnectedSocket->PendingPackages= g_list_append(NowConnectedSocket->PendingPackages,TmpTrollMessage);
            g_info("Package SeqNum %u goint to resend. (Session %u)",TmpTrollMessage->SeqNum,TmpTrollMessage->Session);
            break;
        }
    }
    g_debug("Recv Timer done.");
    return 0;
}

ERRCODE SolveAcceptCallback(){
    g_debug("Sending available accept callback to front process");
    if(WaitingAcceptSocketsList==NULL){
        return 0;
    }
    g_debug("Searching accept request in binded sockets ...");
    uint i;
    for(i=0;i<BindedSockets->len;i++){
        struct BindedSocketStruct *TmpBindedSocket=g_array_index(BindedSockets,struct BindedSocketStruct *,i);
        if(TmpBindedSocket->Accepting){
            g_debug("Searching waiting list with given binded socket ...");
            GList *l;
            for(l=WaitingAcceptSocketsList;l!=NULL;l=l->next){
                struct AcceptedSocketStruct *TmpAcceptedSocket=(struct AcceptedSocketStruct *)(l->data);
                if(TmpAcceptedSocket->SocketFD==TmpBindedSocket->SocketFD){
                    g_debug("Sending accept callback ...");
                    if(sendto(TcpdMsgSocket, (char *)&(TmpAcceptedSocket->Session), sizeof(int), 0, (struct sockaddr *)&(TmpBindedSocket->AcceptingAddr), sizeof(TmpBindedSocket->AcceptingAddr)) <0) {
                        perror("error sending ACCEPT return");
                        exit(4);
                    }
                    g_debug("Moving accepted socket struct from waiting list to accepted list ...");
                    g_hash_table_insert(AcceptedSockets,GUINT_TO_POINTER(TmpAcceptedSocket->Session),TmpAcceptedSocket);
                    WaitingAcceptSocketsList=g_list_remove_link(WaitingAcceptSocketsList,l);
                    TmpBindedSocket->Accepting=FALSE;
                    break;
                }
            }
        }
    }
    g_debug("Solve Accept Callback done.");
    return 0;
}

ERRCODE SolveRecvCallback(){
    g_debug("Sending Recv Callbacks ...");
    GHashTableIter AcceptedSocketIter;
    int AcceptedSocketSession;
    struct AcceptedSocketStruct *TmpAcceptedSocket;
    g_hash_table_iter_init (&AcceptedSocketIter, AcceptedSockets);
    while (g_hash_table_iter_next (&AcceptedSocketIter, (void *)&AcceptedSocketSession, (void *)&TmpAcceptedSocket))
    {
        if(TmpAcceptedSocket->ReceivingLength>0){
            g_debug("Testing RECV request on session %u with length %zu",TmpAcceptedSocket->Session,TmpAcceptedSocket->ReceivingLength);
            if(TmpAcceptedSocket->ReceivingLength>BUF_LEN){
                perror("Request length exceed buffer length");
                exit(5);
            }
            while(TmpAcceptedSocket->RecvBufferSize<TmpAcceptedSocket->ReceivingLength){
                g_debug("RecvBufferSize:%zu RecvBuffer:%s",TmpAcceptedSocket->RecvBufferSize,TmpAcceptedSocket->RecvBuffer);
                if(TmpAcceptedSocket->Packages==NULL){
                    break;
                }
                struct TrollMessageStruct *NowTrollMessage=(struct TrollMessageStruct *)(g_list_first(TmpAcceptedSocket->Packages)->data);
                if(NowTrollMessage->SeqNum!=TmpAcceptedSocket->Base){
                    break;
                }
                if(TmpAcceptedSocket->RecvBufferSize+NowTrollMessage->len<=TmpAcceptedSocket->ReceivingLength){
                    bcopy(NowTrollMessage->body,TmpAcceptedSocket->RecvBuffer+TmpAcceptedSocket->RecvBufferSize,NowTrollMessage->len);
                    // bcopy(TmpAcceptedSocket->RecvBuffer,NowTrollMessage->body,NowTrollMessage->len);
                    // for(int i=0;i<NowTrollMessage->len;i++){
                    //     TmpAcceptedSocket->RecvBuffer[TmpAcceptedSocket->RecvBufferSize+i]=NowTrollMessage->body[i];
                    // }
                    TmpAcceptedSocket->RecvBufferSize+=NowTrollMessage->len;
                    TmpAcceptedSocket->Packages=g_list_remove_link(TmpAcceptedSocket->Packages,g_list_first(TmpAcceptedSocket->Packages));
                    TmpAcceptedSocket->Base++;
                    free(NowTrollMessage);
                }else{
                    size_t RemainSize=TmpAcceptedSocket->ReceivingLength-TmpAcceptedSocket->RecvBufferSize;
                    bcopy(NowTrollMessage->body,TmpAcceptedSocket->RecvBuffer+TmpAcceptedSocket->RecvBufferSize,RemainSize);
                    bcopy(NowTrollMessage->body+RemainSize,NowTrollMessage->body,NowTrollMessage->len-RemainSize);
                    NowTrollMessage->len-=RemainSize;
                }
            }
            g_debug("RecvBufferSize:%zu RecvBuffer:%s",TmpAcceptedSocket->RecvBufferSize,TmpAcceptedSocket->RecvBuffer);
            if(TmpAcceptedSocket->RecvBufferSize==TmpAcceptedSocket->ReceivingLength){
                g_debug("Sending data with size %zu to upper process",TmpAcceptedSocket->RecvBufferSize);
                g_debug("data: %s",TmpAcceptedSocket->RecvBuffer);
                if(sendto(TcpdMsgSocket, TmpAcceptedSocket->RecvBuffer, TmpAcceptedSocket->RecvBufferSize, 0, (struct sockaddr *)&(TmpAcceptedSocket->ReceivingAddr), sizeof(TmpAcceptedSocket->ReceivingAddr)) <0) {
                    perror("error sending RECV return");
                    exit(4);
                }
                TmpAcceptedSocket->RecvBufferSize=0;
                TmpAcceptedSocket->ReceivingLength=0;
            }
        }

    }
    return 0;
}

ERRCODE SolveSendCallback(){
    g_debug("Sending all pending data packages to server");
    GHashTableIter ConnectedSocketIter;
    int ConnectedSocketSocketFD;
    struct ConnectedSocketStruct *TmpConnectedSocket;
    g_hash_table_iter_init (&ConnectedSocketIter, ConnectedSockets);
    while (g_hash_table_iter_next (&ConnectedSocketIter,(void *)&ConnectedSocketSocketFD, (void *)&TmpConnectedSocket))
    {
        g_debug("Updating MinSeq of ConnectedSocket");
        TmpConnectedSocket->MinSeq=100000000;
        for(GList *l=TmpConnectedSocket->Packages;l!=NULL;l=l->next){
            struct TrollMessageStruct *TmpTrollMessage=(struct TrollMessageStruct *)(l->data);
            TmpConnectedSocket->MinSeq=TmpTrollMessage->SeqNum<TmpConnectedSocket->MinSeq?TmpTrollMessage->SeqNum:TmpConnectedSocket->MinSeq;
        }
        GList *l=TmpConnectedSocket->PendingPackages;
        while(l!=NULL){
            g_debug("Sending one pending package ...");
            GList *lnext=l->next;
            struct TrollMessageStruct *NowTrollMessage=(struct TrollMessageStruct *)(l->data);
            if(NowTrollMessage->SeqNum>=TmpConnectedSocket->MinSeq+MAX_BUFFER_PACKAGES){
                l=lnext;
                continue;
            }
            g_info("Sending package Addr:%s, Port:%u, SeqNum:%u (Session:%u)",inet_ntoa(NowTrollMessage->header.sin_addr),ntohs(NowTrollMessage->header.sin_port),NowTrollMessage->SeqNum,NowTrollMessage->Session);
            ssize_t SendRet=sendto(TmpConnectedSocket->SocketFD, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&TrollAddr, sizeof(TrollAddr));
            if(SendRet < 0) {
                perror("error sending buffer to troll");
                exit(5);
            }
            TmpConnectedSocket->Packages= g_list_append(TmpConnectedSocket->Packages,NowTrollMessage);
            TmpConnectedSocket->PendingPackages=g_list_remove_link(TmpConnectedSocket->PendingPackages,l);
            g_debug("Send timer request");
            SendTimerRequest(TimerDriverSocket,&TimerAddr,NowTrollMessage->Session,NowTrollMessage->SeqNum,10);
            g_debug("Delay 1000usec to avoid blocking local udp transmission...");
            usleep(1000);
            // sleep(1);
            l=lnext;
        }
        // while(TmpConnectedSocket->PendingPackages!=NULL){
        //     g_debug("Sending one pending package ...");
        //     struct TrollMessageStruct *NowTrollMessage=(struct TrollMessageStruct *)(g_list_first(TmpConnectedSocket->PendingPackages)->data);
        //     if(NowTrollMessage->SeqNum>=TmpConnectedSocket->MinSeq+MAX_BUFFER_PACKAGES){
        //         continue;
        //     }
        //     g_info("Sending package Addr:%s, Port:%u, SeqNum:%u (Session:%u)",inet_ntoa(NowTrollMessage->header.sin_addr),ntohs(NowTrollMessage->header.sin_port),NowTrollMessage->SeqNum,NowTrollMessage->Session);
        //     ssize_t SendRet=sendto(TmpConnectedSocket->SocketFD, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&TrollAddr, sizeof(TrollAddr));
        //     if(SendRet < 0) {
        //         perror("error sending buffer to troll");
        //         exit(5);
        //     }
        //     TmpConnectedSocket->Packages= g_list_append(TmpConnectedSocket->Packages,NowTrollMessage);
        //     TmpConnectedSocket->PendingPackages=g_list_remove_link(TmpConnectedSocket->PendingPackages,g_list_first(TmpConnectedSocket->PendingPackages));
        //     g_debug("Send timer request");
        //     SendTimerRequest(TimerDriverSocket,&TimerAddr,NowTrollMessage->Session,NowTrollMessage->SeqNum,10);
        //     g_debug("Delay 1000usec to avoid blocking local udp transmission...");
        //     usleep(1000);
        //     // sleep(1);
        // }
    }
    g_debug("Solve Send Callback done.");
    return 0;
}

ERRCODE PrintTcpdStatus(){
    g_debug("Printing Tcpd Status...");
    g_info("##########");
    g_info("#Tcpd Status");
    g_info("##BindedSockets len:%u",BindedSockets->len);
    uint i;
    for(i=0;i<BindedSockets->len;i++){
        struct BindedSocketStruct *TmpBindedSocket=g_array_index(BindedSockets,struct BindedSocketStruct *,i);
        g_info("###BindedSocket:%u SocketFD:%d Accepting:%d Addr:%s Port:%hu",i,TmpBindedSocket->SocketFD,TmpBindedSocket->Accepting,inet_ntoa(TmpBindedSocket->AcceptingAddr.sin_addr),ntohs(TmpBindedSocket->AcceptingAddr.sin_port));
    }
    g_info("##AcceptedSockets size:%u",g_hash_table_size(AcceptedSockets));
    GHashTableIter AcceptedSocketIter;
    int AcceptedSocketSession;
    struct AcceptedSocketStruct *TmpAcceptedSocket;
    g_hash_table_iter_init (&AcceptedSocketIter, AcceptedSockets);
    while (g_hash_table_iter_next (&AcceptedSocketIter, (void *)&AcceptedSocketSession, (void *)&TmpAcceptedSocket))
    {
        g_info("###AcceptedSocket:%d Session:%u Base:%u Packages:%u",TmpAcceptedSocket->SocketFD,TmpAcceptedSocket->Session,TmpAcceptedSocket->Base,g_list_length(TmpAcceptedSocket->Packages));
    }
    g_info("##WaitingAcceptSocketsList size:%u",g_list_length(WaitingAcceptSocketsList));
    GList *l;
    for(l=WaitingAcceptSocketsList;l!=NULL;l=l->next){
        TmpAcceptedSocket=(struct AcceptedSocketStruct *)(l->data);
        g_info("###WaitingSocket:%d Session:%u Base:%u Packages:%u",TmpAcceptedSocket->SocketFD,TmpAcceptedSocket->Session,TmpAcceptedSocket->Base,g_list_length(TmpAcceptedSocket->Packages));
    }
    g_info("##ConnectedSockets size:%u",g_hash_table_size(ConnectedSockets));
    GHashTableIter ConnectedSocketIter;
    int ConnectedSocketSocketFD;
    struct ConnectedSocketStruct *TmpConnectedSocket;
    g_hash_table_iter_init (&ConnectedSocketIter, ConnectedSockets);
    while (g_hash_table_iter_next (&ConnectedSocketIter, (void *)&ConnectedSocketSocketFD, (void *)&TmpConnectedSocket))
    {
        g_info("###ConnectedSocket:%d Session:%u Base:%u Packages:%u Pending:%u",TmpConnectedSocket->SocketFD,TmpConnectedSocket->Session,TmpConnectedSocket->Base,g_list_length(TmpConnectedSocket->Packages),g_list_length(TmpConnectedSocket->PendingPackages));
        // for(l=TmpConnectedSocket->Packages;l!=NULL;l=l->next){
        //     struct TrollMessageStruct *TmpTrollMessage=(struct TrollMessageStruct *)(l->data);
        //     g_info("#### Package %u waiting for response",TmpTrollMessage->SeqNum);
        // }
        // for(l=TmpConnectedSocket->PendingPackages;l!=NULL;l=l->next){
        //     struct TrollMessageStruct *TmpTrollMessage=(struct TrollMessageStruct *)(l->data);
        //     g_info("#### Package %u waiting pending for send",TmpTrollMessage->SeqNum);
        // }
    }
    g_info("#Tcpd Status done.");
    g_info("##########");
    g_debug("Printing Tcp Status done.");
    return 0;
}

ERRCODE tcpd_SOCKET(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength){
    /* fetch the oringin arguments of SOCKET calls */
    // int domain;
    // int type;
    // int protocol;

    // bzero(buf, sizeof(int));
    // FrontAddrLength= sizeof(FrontAddr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
    //     perror("error receiving SOCKET domain"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&domain,sizeof(int));

    // bzero(buf, sizeof(int));
    // FrontAddrLength= sizeof(FrontAddr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
    //     perror("error receiving SOCKET type"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&type,sizeof(int));

    // bzero(buf, sizeof(int));
    // FrontAddrLength= sizeof(FrontAddr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
    //     perror("error receiving SOCKET protocol"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&protocol,sizeof(int));

    g_info("SOCKET");
    // g_info("SOCKET domain:%d, type:%d, protocol:%d",domain,type,protocol);

    /* initialize the socket that upper process calls */
    int ret_sock=socket(AF_INET, SOCK_DGRAM,0);
    if(ret_sock < 0) {
        perror("opening datagram socket");
        exit(2);
    }

    g_info("SOCKET return %d",ret_sock);

    /* return the socket initial result */
    if(sendto(sock, (char *)&ret_sock, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
        perror("error sending SOCKET return");
        exit(4);
    }
    return 0;
}

ERRCODE tcpd_BIND(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength){
    /* fetch the oringin arguments of BIND calls */
    int sockfd;
    struct sockaddr_in addr;
    // socklen_t addrlen;

    bzero(buf, sizeof(int));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving BIND sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(struct sockaddr_in));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(struct sockaddr_in),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving BIND addr"); 
        exit(4);
    }
    bcopy(buf,(char *)&addr,sizeof(struct sockaddr_in));

    // bzero(buf, sizeof(socklen_t));
    // FrontAddrLength= sizeof(FrontAddr);
    // if(recvfrom(sock, buf, sizeof(socklen_t),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
    //     perror("error receiving BIND addrlen"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&addrlen,sizeof(socklen_t));

    g_info("BIND sockfd:%d, addr.port:%hu, addr.in_addr:%s",sockfd,ntohs(addr.sin_port),inet_ntoa(addr.sin_addr));
    // g_info("SOCKET sockfd:%d, addr.port:%hu, addr.in_addr:%lu, addrlen:%d",sockfd,addr.sin_port,addr.sin_addr,addrlen);

    /* bind the socket with name provided */
    int ret_bind=bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret_bind < 0) {
        perror("error binding socket");
        exit(2);
    }

    struct BindedSocketStruct *NowBindedSocket=(struct BindedSocketStruct *)malloc(sizeof(struct BindedSocketStruct));
    NowBindedSocket->SocketFD=sockfd;
    NowBindedSocket->Accepting=FALSE;
    g_array_append_val(BindedSockets,NowBindedSocket);

    g_info("BIND return %d",ret_bind);
    
    /* send bind result back */
    if(sendto(sock, (char *)&ret_bind, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
        perror("error sending BIND return");
        exit(4);
    }
    return 0;
}

ERRCODE tcpd_ACCEPT(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength){
    g_debug("Received ACCEPT message ...");
    int AcceptSocketFD;

    bzero(buf, sizeof(int));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving ACCEPT sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&AcceptSocketFD,sizeof(int));

    g_info("ACCEPT SocketFD:%d",AcceptSocketFD);

    g_debug("Adding ACCEPT request to binded socket list ...");
    ERRCODE AcceptResult=-1;
    uint i;
    for(i=0;i<BindedSockets->len;i++){
        struct BindedSocketStruct *TmpBindedSocket=g_array_index(BindedSockets,struct BindedSocketStruct *,i);
        if(TmpBindedSocket->SocketFD!=AcceptSocketFD){
            continue;
        }
        if(TmpBindedSocket->Accepting){
            g_message("Already Accepting");
            AcceptResult=-2;
            break;
        }
        TmpBindedSocket->Accepting=TRUE;
        TmpBindedSocket->AcceptingAddr=FrontAddr;
        AcceptResult=0;
    }
    if(AcceptResult==-1){
        g_message("No binded socket found");
    }
    if(AcceptResult!=0){
        g_debug("Sending back error ACCEPT");
        int AcceptRet=-1;
        if(sendto(sock, (char *)&AcceptRet, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
            perror("error sending ACCEPT return");
            exit(4);
        }
        return -1;
    }
    g_message("ACCEPT request added");
    g_debug("Tcpd ACCEPT done.");
    return 0;
}

ERRCODE tcpd_RECV(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength){
    g_debug("Received RECV message ...");
    /* fetch the oringin arguments of RECV calls */
    int sockfd;
    // void *buf;
    size_t len;
    // int flags;

    bzero(buf, sizeof(int));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving RECV sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(size_t));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(size_t),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving RECV len"); 
        exit(4);
    }
    bcopy(buf,(char *)&len,sizeof(size_t));

    // bzero(buf, sizeof(int));
    // FrontAddrLength= sizeof(FrontAddr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
    //     perror("error receiving RECV flags"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&flags,sizeof(int));

    g_info("RECV sockfd:%d, len:%zu",sockfd,len);
    // g_info("RECV sockfd:%d, len:%zu, flags:%d",sockfd,len,flags);

    /* request too large */

    g_warning("TODO: RECV");
    ssize_t RecvRet=0;
    if(len==0){
        g_message("No data required for RECV request");
        RecvRet=0;
        if(sendto(sock, (char *)&RecvRet, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
            perror("error sending RECV return");
            exit(4);
        }
        return -1;
    }
    uint32_t Session=sockfd;
    struct AcceptedSocketStruct *NowAcceptedSocket =g_hash_table_lookup(AcceptedSockets,GUINT_TO_POINTER(Session));
    if(NowAcceptedSocket==NULL){
        g_message("No accepted session with %u",Session);
        RecvRet=-2;
        if(sendto(sock, (char *)&RecvRet, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
            perror("error sending RECV return");
            exit(4);
        }
        return -1;
    }
    if(NowAcceptedSocket->ReceivingLength!=0){
        g_message("Already waiting on session %u",Session);
        RecvRet=-3;
        if(sendto(sock, (char *)&RecvRet, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
            perror("error sending RECV return");
            exit(4);
        }
        return -1;
    }

    NowAcceptedSocket->ReceivingLength=len;
    NowAcceptedSocket->ReceivingAddr=FrontAddr;
//     unsigned char success=0;
//     while(!success){
// #ifdef __WITH_TROLL
//         /* unwrap package with troll message structure */
//         struct TrollMessageStruct troll_message;
//         struct sockaddr_in troll_addr;
//         socklen_t troll_addr_len;
//         troll_addr_len=sizeof(troll_addr);
//         ssize_t ret_recv=recvfrom(sockfd, (void *)&troll_message, sizeof(troll_message), MSG_WAITALL, (struct sockaddr *)&troll_addr, &troll_addr_len);
//         if(ret_recv < 0) {
//             perror("error receiving RECV");
//             exit(5);
//         }   
//         if(NowTrollMessage.len>sizeof(NowTrollMessage.body)){
//             g_warning("[GARBLED] Length:%zu",NowTrollMessage.len);
//             continue;
//         }
//         uint32_t CRC32=CHECKSUM_ALGORITHM(0,NowTrollMessage.body,NowTrollMessage.len);
//         if(CRC32!=NowTrollMessage.CRC32){
//             g_warning( "[GARBLED] CRC32: %u, Origin CRC32: %u", CRC32, NowTrollMessage.CRC32);
//             continue;
//         }
//         if(len>NowTrollMessage.len){
//             len=NowTrollMessage.len;
//         }
//         bcopy(NowTrollMessage.body,buf,len);
//         success=1;
// #else
//         /* receive package directly */
//         struct sockaddr_in client_addr;
//         socklen_t client_addr_len;
//         client_addr_len=sizeof(client_addr);
//         ssize_t ret_recv=recvfrom(sockfd, buf, len, MSG_WAITALL, (struct sockaddr *)&client_addr, &client_addr_len);
//         if(ret_recv < 0) {
//             perror("error receiving RECV");
//             exit(5);
//         }
//         if(len>ret_recv){
//             len=ret_recv;
//         }
//         success=1;
// #endif
//     }

    // g_info("RECV return %zd",len);
    
    // /* send package back to upper process */
    // if(sendto(sock, buf, len, 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
    //     perror("error sending RECV return");
    //     exit(4);
    // }
    g_debug("Tcpd RECV done.");
    return 0;
}

ERRCODE tcpd_CONNECT(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength){
    g_debug("Received CONNECT message ...");
    g_debug("Fetching origin arguments of CONNECT ...");
    int sockfd;
    struct sockaddr_in addr;
    // socklen_t addrlen;

    bzero(buf, sizeof(int));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving CONNECT sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(struct sockaddr_in));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(struct sockaddr_in),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving CONNECT addr"); 
        exit(4);
    }
    bcopy(buf,(char *)&addr,sizeof(struct sockaddr_in));

    // bzero(buf, sizeof(socklen_t));
    // FrontAddrLength= sizeof(FrontAddr);
    // if(recvfrom(sock, buf, sizeof(socklen_t),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
    //     perror("error receiving CONNECT addrlen"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&addrlen,sizeof(socklen_t));

    g_info("CONNECT sockfd:%d, addr.port:%hu, addr.in_addr:%s",sockfd,ntohs(addr.sin_port),inet_ntoa(addr.sin_addr));
    /* record the connected name within socket */
    int ConnectRet=0;
    // int ConnectRet=connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    // if(ConnectRet < 0) {
    //     perror("error connecting socket");
    //     exit(2);
    // }

    g_debug("Constructing ConnectedSocketStruct ...");
    struct ConnectedSocketStruct * NowConnectedSocket=(struct ConnectedSocketStruct *)malloc(sizeof(struct ConnectedSocketStruct));
    NowConnectedSocket->SocketFD=sockfd;
    NowConnectedSocket->Session=rand();
    NowConnectedSocket->Base=0;
    NowConnectedSocket->MinSeq=0;
    NowConnectedSocket->Packages=NULL;
    NowConnectedSocket->PendingPackages=NULL;
    NowConnectedSocket->ServerAddr=addr;
    g_hash_table_insert(ConnectedSockets,GINT_TO_POINTER(NowConnectedSocket->SocketFD),NowConnectedSocket);

    g_debug("Sending back conncet result ...");
    /* send back connect result (alwalys success) */
    if(sendto(sock, (char *)&ConnectRet, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
        perror("error sending CONNECT return");
        exit(4);
    }
    g_debug("Tcpd CONNECT done.");
    return 0;
}


ERRCODE tcpd_SEND(struct sockaddr_in FrontAddr,socklen_t FrontAddrLength){
    g_debug("Received SEND message");
    g_debug("Fetching origin arguments of SEND ...");
    int sockfd;
    // void *buf;
    size_t len;
    // int flags;

    bzero(buf, sizeof(int));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving SEND sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&sockfd,sizeof(int));

    bzero(buf, sizeof(size_t));
    FrontAddrLength= sizeof(FrontAddr);
    if(recvfrom(sock, buf, sizeof(size_t),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving SEND len"); 
        exit(4);
    }
    bcopy(buf,(char *)&len,sizeof(size_t));

    // bzero(buf, sizeof(int));
    // FrontAddrLength= sizeof(FrontAddr);
    // if(recvfrom(sock, buf, sizeof(int),MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
    //     perror("error receiving send flags"); 
    //     exit(4);
    // }
    // bcopy(buf,(char *)&flags,sizeof(int));

    g_debug("Fetching send data ...");
    if(BUF_LEN<len){
        perror("error request length larger than buffer");
        exit(5);
    }
    /* fetch data to send from upper process */
    if(recvfrom(sock, buf, len, MSG_WAITALL, (struct sockaddr *)&FrontAddr, &FrontAddrLength) < 0) {
        perror("error receiving SEND buf"); 
        exit(4);
    }

    g_info("SEND sockfd:%d, len:%zu",sockfd,len);

    g_debug("Find connected socket struct ...");
    struct ConnectedSocketStruct * NowConnectedSocket=g_hash_table_lookup(ConnectedSockets,GINT_TO_POINTER(sockfd));
    if(NowConnectedSocket==NULL){
        perror("Socket not connected");
        exit(1);
    }

#ifdef __WITH_TROLL
    /* wrap data with troll message structure */
    g_debug("Constructing troll message package ...");
    struct TrollMessageStruct * NowTrollMessage=(struct TrollMessageStruct *)malloc(sizeof(struct TrollMessageStruct));
    NowTrollMessage->header=NowConnectedSocket->ServerAddr;
    NowTrollMessage->header.sin_family=htons(AF_INET);
    NowTrollMessage->Session=NowConnectedSocket->Session;
    NowTrollMessage->SeqNum=NowConnectedSocket->Base;
    NowConnectedSocket->Base++;
    bcopy(buf,NowTrollMessage->body,len);
    NowTrollMessage->len=len;
    NowTrollMessage->CheckSum=CHECKSUM_ALGORITHM(0,((char *)(NowTrollMessage))+CheckSumOffset,sizeof(*NowTrollMessage)-CheckSumOffset);
    g_info("Checksum is %u", NowTrollMessage->CheckSum);
    g_debug("Appending troll package after now connected socket packages");
    NowConnectedSocket->PendingPackages=g_list_append(NowConnectedSocket->PendingPackages,NowTrollMessage);
    InBufferPackages++;


    // ssize_t ret_send=sendto(sockfd, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&TrollAddr, sizeof(TrollAddr));
    // sleep(1);
    // if(ret_send < 0) {
    //     perror("error sending buffer to troll");
    //     exit(5);
    // }
#else
    /* send data directly */
    // ssize_t ret_send=sendto(sockfd, buf, len, MSG_WAITALL, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr));
    // if(ret_send < 0) {
    //     perror("error sending buffer");
    //     exit(5);
    // }
#endif

    // g_info("SEND return %zd",ret_send);
    
    g_debug("Sending send result alwalys success...");
    ssize_t SendRet=len;
    /* send result back */
    if(sendto(sock, (char *)&SendRet, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
        perror("error sending SEND return");
        exit(4);
    }
    g_debug("Tcpd SEND done.");
    return 0;
}
