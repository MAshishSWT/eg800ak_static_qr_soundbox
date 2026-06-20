/*================================================================
 * Static QR UPI Soundbox - CRC32 Utility
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_crc32.h"

u32 sb_crc32_update(u32 crc, const void *data, u32 length)
{
    const unsigned char *p;
    u32 i;
    u32 bit;

    if ((data == 0) && (length != 0u)) {
        return crc;
    }

    p = (const unsigned char *)data;
    for (i = 0u; i < length; i++) {
        crc ^= (u32)p[i];
        for (bit = 0u; bit < 8u; bit++) {
            if ((crc & 1u) != 0u) {
                crc = (crc >> 1u) ^ 0xEDB88320u;
            } else {
                crc >>= 1u;
            }
        }
    }

    return crc;
}

u32 sb_crc32_compute(const void *data, u32 length)
{
    return sb_crc32_update(SB_CRC32_INITIAL_VALUE, data, length) ^ 0xFFFFFFFFu;
}
