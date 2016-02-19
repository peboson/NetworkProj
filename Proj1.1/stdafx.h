#ifndef _STDAFX_H_
#define _STDAFX_H_


#define SERVER_PUBLIC_PORT "8000"
#define SERVER_LOCAL_PORT "8001"

#define CLIENT_PUBLIC_PORT "8100"
#define CLIENT_LOCAL_PORT "8101"

#define TCPD_MSG_LEN 8
#define BUF_LEN 1000
#define MAX_PATH 260


#define __WITH_TROLL

#include "tcpudp.h"
#include "crc32.h"
#include "checksum.h"



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