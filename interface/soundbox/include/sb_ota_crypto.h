/*================================================================
 * Static QR UPI Soundbox - OTA SHA-256 / HMAC Helpers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_OTA_CRYPTO_H
#define SB_OTA_CRYPTO_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 state[8];
    u64 bit_len;
    u8 data[64];
    u32 data_len;
} sb_ota_sha256_ctx_t;

void sb_ota_sha256_init(sb_ota_sha256_ctx_t *ctx);
sb_status_t sb_ota_sha256_update(sb_ota_sha256_ctx_t *ctx, const u8 *data, u32 len);
sb_status_t sb_ota_sha256_finish(sb_ota_sha256_ctx_t *ctx, u8 hash[32]);
sb_status_t sb_ota_hmac_sha256(const u8 *key, u32 key_len, const u8 *data, u32 data_len, u8 out[32]);
int sb_ota_digest_equal(const u8 *a, const u8 *b, u32 len);

#ifdef __cplusplus
}
#endif

#endif /* SB_OTA_CRYPTO_H */
