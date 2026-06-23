/*================================================================
 * Static QR UPI Soundbox - KAE8 External NOR Flash Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * The Quectel U: filesystem is a module/user filesystem partition.
 * This service handles the separate board-level W25Q64-class SPI NOR flash
 * connected on the KAE8_SQ1 FLASH_* nets.
 *================================================================*/
#ifndef SB_EXTNOR_H
#define SB_EXTNOR_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_EXTNOR_ID_LEN              (3u)
#define SB_EXTNOR_SECTOR_SIZE_BYTES   (4096u)
#define SB_EXTNOR_PAGE_SIZE_BYTES     (256u)
#define SB_EXTNOR_MAX_XFER_BYTES      (65535u)
#define SB_EXTNOR_EXPECTED_MIN_BYTES  (4u * 1024u * 1024u)
#define SB_EXTNOR_EXPECTED_MAX_BYTES  (16u * 1024u * 1024u)

typedef struct {
    int port;
    int mode;
    int clock;
    unsigned char id[SB_EXTNOR_ID_LEN];
    u32 capacity_bytes;
    int ready;
} sb_extnor_info_t;

sb_status_t sb_extnor_init(void);
sb_status_t sb_extnor_get_info(sb_extnor_info_t *info);
sb_status_t sb_extnor_read(u32 address, void *buffer, u32 length);
sb_status_t sb_extnor_write(u32 address, const void *buffer, u32 length);
sb_status_t sb_extnor_erase_sector(u32 address);
sb_status_t sb_extnor_post_status_event(sb_status_t status);
int sb_extnor_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_EXTNOR_H */
