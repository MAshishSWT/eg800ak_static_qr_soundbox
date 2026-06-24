/*================================================================
 * Static QR UPI Soundbox - External W25Q64 NOR Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_EXT_NOR_FLASH_H
#define SB_EXT_NOR_FLASH_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int ready;
    int jedec_valid;
    u8 manufacturer_id;
    u8 memory_type;
    u8 capacity_id;
    u32 size_bytes;
    u32 last_error;
    u32 health_crc;
} sb_ext_nor_status_t;

sb_status_t sb_ext_nor_flash_init(void);
sb_status_t sb_ext_nor_flash_get_status(sb_ext_nor_status_t *status);
sb_status_t sb_ext_nor_flash_read(u32 address, void *buffer, u32 length);
sb_status_t sb_ext_nor_flash_write(u32 address, const void *buffer, u32 length);
sb_status_t sb_ext_nor_flash_erase_sector(u32 address);
sb_status_t sb_ext_nor_flash_factory_rw_test(void);
int sb_ext_nor_flash_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_EXT_NOR_FLASH_H */
