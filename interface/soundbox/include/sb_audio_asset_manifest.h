/*================================================================
 * Static QR UPI Soundbox - External NOR Audio Manifest
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_ASSET_MANIFEST_H
#define SB_AUDIO_ASSET_MANIFEST_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AUDIO_MANIFEST_MAGIC             (0x53424D46u) /* SBMF */
#define SB_AUDIO_MANIFEST_VERSION           (1u)
#define SB_AUDIO_MANIFEST_MAX_ENTRIES       (1600u)
#define SB_AUDIO_MANIFEST_PATH_LEN          (96u)

typedef struct {
    char logical_path[SB_AUDIO_MANIFEST_PATH_LEN];
    u32 offset;
    u32 length;
    u32 crc32;
    char language[4];
} sb_audio_manifest_entry_t;

typedef struct {
    u32 magic;
    u32 version;
    u32 header_size;
    u32 entry_size;
    u32 entry_count;
    u32 assets_base_addr;
    u32 manifest_crc32;
} sb_audio_manifest_header_t;

typedef struct {
    int ready;
    u32 entry_count;
    u32 last_error;
} sb_audio_manifest_status_t;

sb_status_t sb_audio_manifest_init(void);
sb_status_t sb_audio_manifest_get_status(sb_audio_manifest_status_t *status);
sb_status_t sb_audio_manifest_find(const char *logical_path, sb_audio_manifest_entry_t *entry);
sb_status_t sb_audio_manifest_find_with_fallback(const char *logical_path, sb_audio_manifest_entry_t *entry);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_ASSET_MANIFEST_H */
