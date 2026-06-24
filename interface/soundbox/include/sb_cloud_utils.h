/*================================================================
 * Static QR UPI Soundbox - Cloud Utility Helpers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_CLOUD_UTILS_H
#define SB_CLOUD_UTILS_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 sb_cloud_str_len(const char *text);
void sb_cloud_copy_string(char *dst, u32 dst_len, const char *src);
sb_status_t sb_cloud_append_string(char *dst, u32 dst_len, const char *src);
sb_status_t sb_cloud_append_u32(char *dst, u32 dst_len, u32 value);
sb_status_t sb_cloud_append_json_string(char *dst, u32 dst_len, const char *value);
int sb_cloud_has_prefix(const char *text, const char *prefix);
int sb_cloud_text_equal(const char *a, const char *b);
int sb_cloud_find(const char *text, const char *needle);
int sb_cloud_url_is_https(const char *url);

#ifdef __cplusplus
}
#endif

#endif /* SB_CLOUD_UTILS_H */
