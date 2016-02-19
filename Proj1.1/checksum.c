#include "checksum.h"
uint32_t  checksum(void* buf, size_t len){
    uint64_t sum=0;
    uint32_t *buf_uint32=buf;
    while(len){
        sum+=(*buf_uint32);
        buf_uint32++;
        len-=4;
    }
    uint8_t *buf_uint8=buf_uint32;
    size_t left=len%4;
    uint32_t remain=0;
    for(size_t i=0;i<4;i++){
        remain=(remain<<8);
        if(len){
            remain+=*buf_uint8;
            buf_uint8++;
            len--;
        }
    }
    sum+=remain;
    return (uint32_t)((sum>>32)+sum);
}
uint32_t deprecated_crc32(void* buf, size_t len){
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