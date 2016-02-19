#include "checksum.h"
uint32_t  checksum(void* buf, size_t len){
    uint64_t sum=0;
    uint32_t *buf_uint32=buf;
    size_t loops=len/sizeof(uint32_t);
    while(loops--){
        sum+=*buf_uint32;
        buf_uint32++;
    }
    uint8_t *buf_uint8=buf_uint32;
    size_t left=len%4;
    uint32_t remain=0;
    for(size_t i=0;i<4;i++){
        remain=(remain<<8);
        if(left--){
            remain+=*buf_uint8;
            buf_uint8++;
        }
    }
    return (uint32_t)((sum>>32)+sum);
}
uint32_t crc32(void* buf, size_t len){
    uint8_t *buf_uint8=buf;
    uint32_t POLY=0x04C11DB7;
    uint32_t crc=0xFFFFFFFF;
    size_t i;
    while(len--){
        crc=crc^(buf_uint8<<24);
        buf_uint8++;
        for(i=0;i<8;i++){
            if(crc&0x80000000){
                crc=(crc<<1)^POLY;
            }else{
                crc<<=1;
            }
        }
        crc&=0xFFFFFFFF;
    }
    return crc;
}
uint16_t crc16(unsigned char *addr, int num, uint16_t crc)  
{  
    int i;  
    for (; num > 0; num--)              /* Step through bytes in memory */  
    {  
        crc = crc ^ (*addr++ << 8);     /* Fetch byte from memory, XOR into CRC top byte*/  
        for (i = 0; i < 8; i++)             /* Prepare to rotate 8 bits */  
        {  
            if (crc & 0x8000)            /* b15 is set... */  
                crc = (crc << 1) ^ POLY;    /* rotate and XOR with polynomic */  
            else                          /* b15 is clear... */  
                crc <<= 1;                  /* just rotate */  
        }                             /* Loop for 8 bits */  
        crc &= 0xFFFF;                  /* Ensure CRC remains 16-bit value */  
    }                               /* Loop until num=0 */  
    return(crc);                    /* Return updated CRC */  
}  