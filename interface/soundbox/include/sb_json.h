/*================================================================
 * Static QR UPI Soundbox - Minimal JSON Helpers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_JSON_H
#define SB_JSON_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

sb_status_t sb_json_get_string(const char *json, const char *key, char *out, u32 out_len);
sb_status_t sb_json_get_u64(const char *json, const char *key, u64 *out);
sb_status_t sb_json_get_amount_paise(const char *json, u64 *amount_paise);

#ifdef __cplusplus
}
#endif

#endif /* SB_JSON_H */
