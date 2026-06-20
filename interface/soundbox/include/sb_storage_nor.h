/*================================================================
 * Static QR UPI Soundbox - External SPI NOR Abstraction
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_STORAGE_NOR_H
#define SB_STORAGE_NOR_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_NOR_ID_LEN          (3u)
#define SB_NOR_SECTOR_SIZE     (4096u)
#define SB_NOR_MAX_XFER_BYTES  (65535u)

typedef struct {
    int port;
    unsigned char id[SB_NOR_ID_LEN];
    int ready;
} sb_nor_info_t;

sb_status_t sb_storage_nor_init(void);
sb_status_t sb_storage_nor_get_info(sb_nor_info_t *info);
sb_status_t sb_storage_nor_read(u32 address, void *buffer, u32 length);
sb_status_t sb_storage_nor_write(u32 address, const void *buffer, u32 length);
sb_status_t sb_storage_nor_erase_sector(u32 address);
int sb_storage_nor_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_STORAGE_NOR_H */
