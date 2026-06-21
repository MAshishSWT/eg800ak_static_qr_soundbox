/*================================================================
 * Static QR UPI Soundbox - OTA Manifest
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_OTA_MANIFEST_H
#define SB_OTA_MANIFEST_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_OTA_TYPE_LEN             (16u)
#define SB_OTA_VERSION_LEN          (32u)
#define SB_OTA_URL_LEN              (192u)
#define SB_OTA_SHA256_HEX_LEN       (65u)
#define SB_OTA_SIGNATURE_HEX_LEN    (65u)
#define SB_OTA_TARGET_PATH_LEN      (96u)
#define SB_OTA_CANONICAL_LEN        (384u)
#define SB_OTA_SECRET_INDEX         (1u)
#define SB_OTA_HMAC_KEY_LEN         (32u)

typedef enum {
    SB_OTA_KIND_FIRMWARE = 0,
    SB_OTA_KIND_AUDIO_PACK = 1
} sb_ota_kind_t;

typedef struct {
    sb_ota_kind_t kind;
    char type[SB_OTA_TYPE_LEN];
    char version[SB_OTA_VERSION_LEN];
    char url[SB_OTA_URL_LEN];
    u32 size_bytes;
    char sha256_hex[SB_OTA_SHA256_HEX_LEN];
    char signature_hex[SB_OTA_SIGNATURE_HEX_LEN];
    char target_path[SB_OTA_TARGET_PATH_LEN];
} sb_ota_manifest_t;

sb_status_t sb_ota_manifest_parse(const char *json, sb_ota_manifest_t *manifest);
sb_status_t sb_ota_manifest_verify_signature(const sb_ota_manifest_t *manifest);
sb_status_t sb_ota_manifest_sha_hex_to_bytes(const char *hex, unsigned char out[32]);

#ifdef __cplusplus
}
#endif

#endif /* SB_OTA_MANIFEST_H */
