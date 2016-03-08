#include <stdint.h>
#include <stddef.h>
uint32_t checksum32(uint32_t crc, void* buf, size_t len);
uint32_t deprecated_crc32(uint32_t crc, void* buf, size_t len);
