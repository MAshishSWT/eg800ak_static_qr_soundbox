/*================================================================
 * Static QR UPI Soundbox - OTA Manifest
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_type.h"
#include "ql_securedata.h"
#include "sb_cloud_utils.h"
#include "sb_json.h"
#include "sb_ota_crypto.h"
#include "sb_ota_manifest.h"

#define SB_OTA_HMAC_BLOCK_LEN       (64u)
#define SB_OTA_HMAC_DIGEST_LEN      (32u)

static void sb_ota_zero(void *ptr, u32 len)
{
    u32 i;
    unsigned char *p = (unsigned char *)ptr;

    if (p == 0) {
        return;
    }
    for (i = 0u; i < len; i++) {
        p[i] = 0u;
    }
}


static int sb_ota_key_has_data(const unsigned char *key, u32 len)
{
    u32 i;
    unsigned char acc = 0u;

    if (key == 0) {
        return 0;
    }
    for (i = 0u; i < len; i++) {
        acc = (unsigned char)(acc | key[i]);
    }
    return (acc != 0u) ? 1 : 0;
}

static int sb_ota_hex_value(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return (int)(c - '0');
    }
    if ((c >= 'a') && (c <= 'f')) {
        return (int)(c - 'a' + 10);
    }
    if ((c >= 'A') && (c <= 'F')) {
        return (int)(c - 'A' + 10);
    }
    return -1;
}

sb_status_t sb_ota_manifest_sha_hex_to_bytes(const char *hex, unsigned char out[32])
{
    u32 i;

    if ((hex == 0) || (out == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_cloud_str_len(hex) != 64u) {
        return SB_STATUS_INVALID_PARAM;
    }

    for (i = 0u; i < 32u; i++) {
        int hi = sb_ota_hex_value(hex[i * 2u]);
        int lo = sb_ota_hex_value(hex[(i * 2u) + 1u]);
        if ((hi < 0) || (lo < 0)) {
            return SB_STATUS_INVALID_PARAM;
        }
        out[i] = (unsigned char)(((u32)hi << 4u) | (u32)lo);
    }

    return SB_STATUS_OK;
}



static sb_status_t sb_ota_build_canonical(const sb_ota_manifest_t *manifest, char *out, u32 out_len)
{
    if ((manifest == 0) || (out == 0) || (out_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    out[0] = '\0';
    if (sb_cloud_append_string(out, out_len, manifest->type) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(out, out_len, "|") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(out, out_len, manifest->version) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(out, out_len, "|") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(out, out_len, manifest->url) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(out, out_len, "|") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_u32(out, out_len, manifest->size_bytes) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(out, out_len, "|") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(out, out_len, manifest->sha256_hex) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (manifest->target_path[0] != '\0') {
        if (sb_cloud_append_string(out, out_len, "|") != SB_STATUS_OK) {
            return SB_STATUS_NO_MEMORY;
        }
        if (sb_cloud_append_string(out, out_len, manifest->target_path) != SB_STATUS_OK) {
            return SB_STATUS_NO_MEMORY;
        }
    }

    return SB_STATUS_OK;
}



sb_status_t sb_ota_manifest_parse(const char *json, sb_ota_manifest_t *manifest)
{
    u64 size64 = 0u;

    if ((json == 0) || (manifest == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    sb_ota_zero(manifest, (u32)sizeof(*manifest));

    if (sb_json_get_string(json, "type", manifest->type, SB_OTA_TYPE_LEN) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_json_get_string(json, "version", manifest->version, SB_OTA_VERSION_LEN) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_json_get_string(json, "url", manifest->url, SB_OTA_URL_LEN) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_json_get_u64(json, "size", &size64) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if ((size64 == 0u) || (size64 > 0xFFFFFFFFu)) {
        return SB_STATUS_INVALID_PARAM;
    }
    manifest->size_bytes = (u32)size64;

    if (sb_json_get_string(json, "sha256", manifest->sha256_hex, SB_OTA_SHA256_HEX_LEN) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_json_get_string(json, "signature", manifest->signature_hex, SB_OTA_SIGNATURE_HEX_LEN) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    (void)sb_json_get_string(json, "target_path", manifest->target_path, SB_OTA_TARGET_PATH_LEN);

    if (sb_cloud_text_equal(manifest->type, "firmware") != 0) {
        manifest->kind = SB_OTA_KIND_FIRMWARE;
    } else if ((sb_cloud_text_equal(manifest->type, "audio_pack") != 0) ||
               (sb_cloud_text_equal(manifest->type, "audio") != 0)) {
        manifest->kind = SB_OTA_KIND_AUDIO_PACK;
    } else {
        return SB_STATUS_UNSUPPORTED;
    }

    {
        unsigned char digest[32];
        if (sb_ota_manifest_sha_hex_to_bytes(manifest->sha256_hex, digest) != SB_STATUS_OK) {
            return SB_STATUS_INVALID_PARAM;
        }
    }

    return SB_STATUS_OK;
}

sb_status_t sb_ota_manifest_verify_signature(const sb_ota_manifest_t *manifest)
{
    unsigned char key[SB_OTA_HMAC_KEY_LEN];
    unsigned char expected[32];
    unsigned char actual[32];
    char canonical[SB_OTA_CANONICAL_LEN];
    sb_status_t status;
    int ret;

    if (manifest == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (sb_ota_manifest_sha_hex_to_bytes(manifest->signature_hex, expected) != SB_STATUS_OK) {
        return SB_STATUS_SECURITY_ERROR;
    }

    sb_ota_zero(key, (u32)sizeof(key));
    ret = ql_securedata_read(SB_OTA_SECRET_INDEX, key, SB_OTA_HMAC_KEY_LEN);
    if ((ret < 0) || (sb_ota_key_has_data(key, SB_OTA_HMAC_KEY_LEN) == 0)) {
        return SB_STATUS_SECURITY_ERROR;
    }

    status = sb_ota_build_canonical(manifest, canonical, (u32)sizeof(canonical));
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_ota_hmac_sha256(key,
                                SB_OTA_HMAC_KEY_LEN,
                                (const u8 *)canonical,
                                sb_cloud_str_len(canonical),
                                actual);
    sb_ota_zero(key, (u32)sizeof(key));
    if (status != SB_STATUS_OK) {
        return status;
    }

    return (sb_ota_digest_equal(expected, actual, 32u) != 0) ? SB_STATUS_OK : SB_STATUS_SECURITY_ERROR;
}
