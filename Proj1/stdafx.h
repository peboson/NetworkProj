#ifndef _STDAFX_H_
#define _STDAFX_H_

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

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>

#define SERVER_PUBLIC_PORT "8000"
#define SERVER_LOCAL_PORT "8001"

#define CLIENT_PUBLIC_PORT "8100"
#define CLIENT_LOCAL_PORT "8101"
#define TIMER_PORT "8080"

#define TCPD_MSG_LEN 8
#define BUF_LEN 1000
#define MAX_PATH 260

#define CHECKSUM_ALGORITHM checksum32

#define SEC2USEC 1000000
#define USEC2SEC 0.000001

#define ERRCODE int32_t



#define __WITH_TROLL


#include "tcpudp.h"
#include "crc32.h"
#include "checksum.h"
#include "TimerRequest.h"
#include "Global.h"



#ifdef __WITH_TROLL
#define TROLL_PORT "8081"
#include "Troll/troll.h"
/* the wrap of troll message */
struct troll_message_t{
    struct sockaddr_in header;
    uint32_t CRC32;
    size_t len;
    char body[BUF_LEN];
};
#endif



#endif