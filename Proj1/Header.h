#ifndef _HEADER_H_
#define _HEADER_H_

#include <netdb.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>

#define SERVER_PUBLIC_PORT "8000"
#define SERVER_TCPD_PORT "8001"

#define CLIENT_PUBLIC_PORT "8100"
#define CLIENT_TCPD_PORT "8101"

#define TIMER_PORT "8080"

#define TCPD_MSG_LEN 8
#define BUF_LEN 1000
#define MAX_PATH 260
#define MAX_BUFFER_PACKAGES 100

#define CHECKSUM_ALGORITHM crc32

#define SEC2USEC 1000000
#define USEC2SEC 0.000001

#define ERRCODE int32_t

#define __WITH_TROLL

#ifdef __WITH_TROLL
#define TROLL_PORT "8081"
#include "Troll/troll.h"
/* the wrap of troll message */
struct TrollMessageStruct{
    struct sockaddr_in header;
    uint32_t CheckSum;
    uint32_t Session;
    uint32_t SeqNum;
    size_t len;
    char body[BUF_LEN];
};
#define troll_message_t TrollMessageStruct
#define CRC32 CheckSum
#define CheckSumOffset (sizeof(struct sockaddr_in)+sizeof(uint32_t))
struct TrollAckStruct{
    struct sockaddr_in header;
    uint32_t CheckSum;
    uint32_t Session;
    uint32_t SeqNum;
};
#define TrollAckCheckSumOffset (sizeof(struct sockaddr_in)+sizeof(uint32_t))

#endif

#endif
