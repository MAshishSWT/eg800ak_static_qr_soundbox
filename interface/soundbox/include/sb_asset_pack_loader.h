/*================================================================
 * Static QR UPI Soundbox - External NOR Audio Asset Pack Loader
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_ASSET_PACK_LOADER_H
#define SB_ASSET_PACK_LOADER_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_ASSET_PACK_MAGIC              (0x53424153u) /* SBAS */
#define SB_ASSET_PACK_HEADER_BYTES       (12u)
#define SB_ASSET_PACK_INDEX_ENTRY_BYTES  (112u)
#define SB_ASSET_PACK_MAX_BYTES          (8u * 1024u * 1024u)
#define SB_ASSET_PACK_UART_HEX_MAX_BYTES (1024u)
#define SB_ASSET_PACK_VERIFY_CHUNK_BYTES (1024u)

typedef struct {
    int active;
    u32 expected_size;
    u32 expected_crc32;
    u32 received_size;
    u32 running_crc32;
    u32 finalized_crc32;
    u32 header_magic;
    u8 header_bytes[4];
    u32 last_error;
} sb_asset_pack_loader_status_t;

sb_status_t sb_asset_pack_loader_init(void);
sb_status_t sb_asset_pack_loader_begin(u32 expected_size, u32 expected_crc32, u32 erase);
sb_status_t sb_asset_pack_loader_write(u32 offset, const u8 *data, u32 length);
sb_status_t sb_asset_pack_loader_end(void);
sb_status_t sb_asset_pack_loader_abort(void);
sb_status_t sb_asset_pack_loader_status(sb_asset_pack_loader_status_t *status);
sb_status_t sb_asset_pack_loader_write_hex(u32 offset, const char *hex_data, u32 *written_bytes);
sb_status_t sb_asset_pack_loader_ftp_get(const char *host,
                                          const char *user,
                                          const char *password,
                                          const char *remote_file,
                                          u32 cid,
                                          u32 expected_size,
                                          u32 expected_crc32);

#ifdef __cplusplus
}
#endif

#endif /* SB_ASSET_PACK_LOADER_H */
