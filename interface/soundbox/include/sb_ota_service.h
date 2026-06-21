/*================================================================
 * Static QR UPI Soundbox - OTA Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_OTA_SERVICE_H
#define SB_OTA_SERVICE_H

#include "ql_type.h"
#include "sb_config.h"
#include "sb_error.h"
#include "sb_ota_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_OTA_TASK_STACK_SIZE_BYTES       (14u * 1024u)
#define SB_OTA_TASK_PRIORITY               (19u)
#define SB_OTA_QUEUE_DEPTH                 (2u)
#define SB_OTA_MANIFEST_JSON_LEN           (512u)
#define SB_OTA_HTTP_TIMEOUT_MS             (60000u)
#define SB_OTA_PDP_PROFILE_ID              (1)
#define SB_OTA_TLS_PORT                    (443u)
#define SB_OTA_AUDIO_TEMP_PATH             "U:/sb_audio_pack.tmp"
#define SB_OTA_AUDIO_ACTIVE_PATH           "U:/sb_audio_pack.bin"
#define SB_OTA_AUDIO_STATE_PATH            "U:/sb_audio_state.json"
#define SB_OTA_FW_PACKAGE_PATH             "U:/FotaFile.bin"

typedef enum {
    SB_OTA_STATE_IDLE = 0,
    SB_OTA_STATE_QUEUED,
    SB_OTA_STATE_VERIFY_MANIFEST,
    SB_OTA_STATE_DOWNLOADING,
    SB_OTA_STATE_VERIFY_IMAGE,
    SB_OTA_STATE_STAGED,
    SB_OTA_STATE_FAULT
} sb_ota_state_t;

typedef struct {
    sb_ota_state_t state;
    sb_ota_kind_t kind;
    u32 progress_percent;
    int last_error;
    char version[SB_OTA_VERSION_LEN];
} sb_ota_status_t;

sb_status_t sb_ota_service_init(const sb_config_payload_t *config);
sb_status_t sb_ota_service_start_from_manifest_json(const char *manifest_json);
sb_status_t sb_ota_get_status(sb_ota_status_t *status);
const char *sb_ota_state_name(sb_ota_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* SB_OTA_SERVICE_H */
