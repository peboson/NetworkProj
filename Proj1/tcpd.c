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

struct hostent *troll_hp; /* troll server host */

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
    GList *Packages;
};
GHashTable *AcceptedSockets;
GList *WaitingAcceptSocketsList;

struct ConnectedSocketStruct{
    int SocketFD;
    uint32_t Session;
    uint32_t Base;
    struct sockaddr_in ServerAddr;
    GList *Packages;
    GList *PendingPackages;
};
GHashTable *ConnectedSockets;

ERRCODE RecvData(struct BindedSocketStruct *NowBindedSocket);
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
        g_warning("USAGE: ftpd server|client|port\n");
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

#ifdef __WITH_TROLL
    /* fetch troll server host via host name (local host) */
    troll_hp = gethostbyname("localhost");
    if(troll_hp == 0) {
        g_warning( "unknown host: localhost\n");
        exit(2);
    }
    /* construct name of socket to send to */
    bcopy((void *)troll_hp->h_addr, (void *)&TrollAddr.sin_addr, troll_hp->h_length);
    TrollAddr.sin_family = AF_INET;
    TrollAddr.sin_port = htons(atoi(TROLL_PORT));
#endif

    g_info("Server waiting on port # %d\n", ntohs(tcpd_name.sin_port));

    /* main loop, drived by upper local process */
    fd_set WaitingSocketSet;
    while(1){
        g_debug("Filling waiting sockets set ...");
        FD_ZERO(&WaitingSocketSet);
        FD_SET(TcpdMsgSocket,&WaitingSocketSet);
        int MaxSocket=TcpdMsgSocket;
        uint i;
        for(i=0;i<BindedSockets->len;i++){
            struct BindedSocketStruct *TmpBindedSocket=g_array_index(BindedSockets,struct BindedSocketStruct *,i);
            FD_SET(TmpBindedSocket->SocketFD,&WaitingSocketSet);
            MaxSocket=TmpBindedSocket->SocketFD>MaxSocket?TmpBindedSocket->SocketFD:MaxSocket;
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
                g_debug("Server receives: %s\n", tcpd_msg);
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
    ssize_t ret_recv=recvfrom(NowBindedSocket->SocketFD, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&RemoteTrollAddr, &RemoteTrollAddrLength);
#else
    struct sockaddr_in ClientAddr;
    socklen_t ClientAddrLength=sizeof(ClientAddr);
    g_debug("Receiving data directly from client ...");
    ssize_t ret_recv=recvfrom(NowBindedSocket->SocketFD, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&ClientAddr, &ClientAddrLength);
    NowTrollMessage->header=ClientAddr;
#endif
    if(ret_recv < 0) {
        perror("error receiving RECV");
        exit(5);
    }   
    g_debug("Checking package length is garbled or not ...");
    if(NowTrollMessage->len>sizeof(NowTrollMessage->body)){
        g_info("[GARBLED] Length:%zu\n",NowTrollMessage->len);
        return -1;
    }
    g_debug("Checking package body is garbled or not ...");
    uint32_t CheckSum=CHECKSUM_ALGORITHM(0,NowTrollMessage->body-CheckSumOffset,NowTrollMessage->len+CheckSumOffset);
    if(CheckSum!=NowTrollMessage->CheckSum){
        g_info("[GARBLED] CheckSum: %u, Origin CheckSum: %u\n", CheckSum, NowTrollMessage->CheckSum);
        return -2;
    }
    g_debug("Finding Accepted socket with session %u",NowTrollMessage->Session);
    struct AcceptedSocketStruct *NowAcceptedSocket=NULL;
    if(g_hash_table_contains(AcceptedSockets,GUINT_TO_POINTER(NowTrollMessage->Session))){
        NowAcceptedSocket=g_hash_table_lookup(AcceptedSockets,GUINT_TO_POINTER(NowTrollMessage->Session));
    }else{
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
        WaitingAcceptSocketsList=g_list_append(WaitingAcceptSocketsList,NowAcceptedSocket);
    }

    g_debug("Inserting received package into packages buffer ...");
    if(NowAcceptedSocket->Base>NowTrollMessage->SeqNum){
        g_info("[OUTDATED] Received Outdated package");
        return -3;
    }
    GList *l;
    for(l=NowAcceptedSocket->Packages;l!=NULL;l=l->next){
        if(((struct TrollMessageStruct *)(l->data))->SeqNum==NowTrollMessage->SeqNum){
            g_info("[DUPLICATED] Received duplicated package");
            return -4;
        }
        if(((struct TrollMessageStruct *)(l->data))->SeqNum>NowTrollMessage->SeqNum){
            break;
        }
    }
    NowAcceptedSocket->Packages=g_list_insert_before(NowAcceptedSocket->Packages,l,NowTrollMessage);
    g_debug("Inserted received package");
    g_debug("Recv Data done.");
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
    g_debug("[NOT IMPLEMENT] TODO: Sending Recv Callbacks ...");
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
        while(TmpConnectedSocket->PendingPackages!=NULL){
            g_debug("Sending one pending package ...");
            struct TrollMessageStruct *NowTrollMessage=(struct TrollMessageStruct *)(g_list_first(TmpConnectedSocket->PendingPackages)->data);
            ssize_t SendRet=sendto(TmpConnectedSocket->SocketFD, (void *)NowTrollMessage, sizeof(*NowTrollMessage), MSG_WAITALL, (struct sockaddr *)&TrollAddr, sizeof(TrollAddr));
            if(SendRet < 0) {
                perror("error sending buffer to troll");
                exit(5);
            }
            TmpConnectedSocket->Packages= g_list_append(TmpConnectedSocket->Packages,NowTrollMessage);
            TmpConnectedSocket->PendingPackages=g_list_remove_link(TmpConnectedSocket->PendingPackages,g_list_first(TmpConnectedSocket->PendingPackages));
            g_info("TODO: add resend timer");
            sleep(1);
        }
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
    // g_info("SOCKET domain:%d, type:%d, protocol:%d\n",domain,type,protocol);

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

    g_info("BIND sockfd:%d, addr.port:%hu, addr.in_addr:%lu\n",sockfd,addr.sin_port,(unsigned long)addr.sin_addr.s_addr);
    // g_info("SOCKET sockfd:%d, addr.port:%hu, addr.in_addr:%lu, addrlen:%d\n",sockfd,addr.sin_port,addr.sin_addr,addrlen);

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

    g_info("BIND return %d\n",ret_bind);
    
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
        perror("error receiving BIND sockfd"); 
        exit(4);
    }
    bcopy(buf,(char *)&AcceptSocketFD,sizeof(int));

    g_debug("ACCEPT SocketFD:%d",AcceptSocketFD);

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
    }
    if(AcceptResult==-1){
        g_message("No binded socket found");
    }
    if(AcceptResult!=0){
        g_debug("Sending back error ACCEPT");
        int AcceptRet=-1;
        if(sendto(sock, (char *)&AcceptRet, sizeof(int), 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
            perror("error sending BIND return");
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

    g_info("RECV sockfd:%d, len:%zu\n",sockfd,len);
    // g_info("RECV sockfd:%d, len:%zu, flags:%d\n",sockfd,len,flags);

    /* request too large */
    if(BUF_LEN<len){
        perror("error request length larger than buffer");
        exit(5);
    }

    g_warning("TODO: RECV");
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
//             g_warning("[GARBLED] Length:%zu\n",NowTrollMessage.len);
//             continue;
//         }
//         uint32_t CRC32=CHECKSUM_ALGORITHM(0,NowTrollMessage.body,NowTrollMessage.len);
//         if(CRC32!=NowTrollMessage.CRC32){
//             g_warning( "[GARBLED] CRC32: %u, Origin CRC32: %u\n", CRC32, NowTrollMessage.CRC32);
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

    g_info("RECV return %zd\n",len);
    
    /* send package back to upper process */
    if(sendto(sock, buf, len, 0, (struct sockaddr *)&FrontAddr, sizeof(FrontAddr)) <0) {
        perror("error sending RECV return");
        exit(4);
    }
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
    int ConnectRet=connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ConnectRet < 0) {
        perror("error connecting socket");
        exit(2);
    }

    g_debug("Constructing ConnectedSocketStruct ...");
    struct ConnectedSocketStruct * NowConnectedSocket=(struct ConnectedSocketStruct *)malloc(sizeof(struct ConnectedSocketStruct));
    NowConnectedSocket->SocketFD=sockfd;
    NowConnectedSocket->Session=rand();
    NowConnectedSocket->Base=0;
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
    struct ConnectedSocketStruct * NowConnectedSocket=NULL;
    if(g_hash_table_contains(ConnectedSockets,GINT_TO_POINTER(sockfd))){
        NowConnectedSocket=g_hash_table_lookup(ConnectedSockets,GINT_TO_POINTER(sockfd));
    }else{
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
    NowTrollMessage->CheckSum=CHECKSUM_ALGORITHM(0,NowTrollMessage->body,NowTrollMessage->len);
    g_info("Checksum is %u\n", NowTrollMessage->CheckSum);
    g_debug("Appending troll package after now connected socket packages");
    NowConnectedSocket->PendingPackages=g_list_append(NowConnectedSocket->PendingPackages,NowTrollMessage);


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

    // g_info("SEND return %zd\n",ret_send);
    
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
