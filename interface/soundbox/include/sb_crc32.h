/*================================================================
 * Static QR UPI Soundbox - CRC32 Utility
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_CRC32_H
#define SB_CRC32_H

#include "ql_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_CRC32_INITIAL_VALUE  (0xFFFFFFFFu)

u32 sb_crc32_update(u32 crc, const void *data, u32 length);
u32 sb_crc32_compute(const void *data, u32 length);

#ifdef __cplusplus
}
#endif

#endif /* SB_CRC32_H */
