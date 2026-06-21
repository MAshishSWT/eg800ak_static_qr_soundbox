/*================================================================
 * Static QR UPI Soundbox - Audio Asset Store Abstraction
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_ASSET_STORE_H
#define SB_AUDIO_ASSET_STORE_H

#include "ql_type.h"
#include "sb_audio_types.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AUDIO_STORE_LOGICAL_ROOT       "audio"
#define SB_AUDIO_STORE_UFS_ROOT           "U:/audio"
#define SB_AUDIO_STORE_STAGE_PATH         "U:/sb_play.mp3"
#define SB_AUDIO_STORE_MAX_STAGE_BYTES    (64u * 1024u)

typedef enum {
    SB_AUDIO_STORE_BACKEND_UFS = 0,
    SB_AUDIO_STORE_BACKEND_EXTNOR
} sb_audio_store_backend_t;

typedef struct {
    int ready;
    int extnor_available;
    u32 missing_count;
    u32 staged_count;
    sb_audio_store_backend_t backend;
} sb_audio_store_status_t;

sb_status_t sb_audio_asset_store_init(void);
sb_status_t sb_audio_asset_store_get_status(sb_audio_store_status_t *status);
sb_status_t sb_audio_asset_store_prepare_play_path(const char *logical_path,
                                                    char *play_path,
                                                    u32 play_path_len);
int sb_audio_asset_store_exists(const char *logical_path);
const char *sb_audio_asset_store_backend_name(sb_audio_store_backend_t backend);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_ASSET_STORE_H */
