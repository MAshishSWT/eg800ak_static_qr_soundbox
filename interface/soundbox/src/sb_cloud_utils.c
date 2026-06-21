/*================================================================
 * Static QR UPI Soundbox - Cloud Utility Helpers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_cloud_utils.h"

u32 sb_cloud_str_len(const char *text)
{
    u32 len = 0u;

    if (text == 0) {
        return 0u;
    }

    while (text[len] != '\0') {
        len++;
    }

    return len;
}

void sb_cloud_copy_string(char *dst, u32 dst_len, const char *src)
{
    u32 i;

    if ((dst == 0) || (dst_len == 0u)) {
        return;
    }

    if (src == 0) {
        dst[0] = '\0';
        return;
    }

    for (i = 0u; (i + 1u < dst_len) && (src[i] != '\0'); i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

sb_status_t sb_cloud_append_string(char *dst, u32 dst_len, const char *src)
{
    u32 used;
    u32 i;

    if ((dst == 0) || (dst_len == 0u) || (src == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    used = sb_cloud_str_len(dst);
    if (used >= dst_len) {
        return SB_STATUS_INVALID_PARAM;
    }

    for (i = 0u; src[i] != '\0'; i++) {
        if ((used + 1u) >= dst_len) {
            dst[dst_len - 1u] = '\0';
            return SB_STATUS_NO_MEMORY;
        }
        dst[used] = src[i];
        used++;
        dst[used] = '\0';
    }

    return SB_STATUS_OK;
}

sb_status_t sb_cloud_append_u32(char *dst, u32 dst_len, u32 value)
{
    char tmp[11];
    u32 pos = 0u;
    u32 div = 1000000000u;
    int started = 0;

    if ((dst == 0) || (dst_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (value == 0u) {
        return sb_cloud_append_string(dst, dst_len, "0");
    }

    while (div > 0u) {
        u32 digit = value / div;
        if ((digit != 0u) || (started != 0)) {
            if (pos + 1u >= (u32)sizeof(tmp)) {
                return SB_STATUS_NO_MEMORY;
            }
            tmp[pos++] = (char)('0' + digit);
            started = 1;
        }
        value = value % div;
        div = div / 10u;
    }
    tmp[pos] = '\0';

    return sb_cloud_append_string(dst, dst_len, tmp);
}

sb_status_t sb_cloud_append_json_string(char *dst, u32 dst_len, const char *value)
{
    u32 i;
    sb_status_t status;

    if ((dst == 0) || (value == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_cloud_append_string(dst, dst_len, "\"");
    if (status != SB_STATUS_OK) {
        return status;
    }

    for (i = 0u; value[i] != '\0'; i++) {
        char ch[2];
        if ((value[i] == '\"') || (value[i] == '\\')) {
            status = sb_cloud_append_string(dst, dst_len, "\\");
            if (status != SB_STATUS_OK) {
                return status;
            }
        }
        if (((unsigned char)value[i]) < 0x20u) {
            continue;
        }
        ch[0] = value[i];
        ch[1] = '\0';
        status = sb_cloud_append_string(dst, dst_len, ch);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    return sb_cloud_append_string(dst, dst_len, "\"");
}

int sb_cloud_has_prefix(const char *text, const char *prefix)
{
    u32 i;

    if ((text == 0) || (prefix == 0)) {
        return 0;
    }

    for (i = 0u; prefix[i] != '\0'; i++) {
        if (text[i] != prefix[i]) {
            return 0;
        }
    }

    return 1;
}

int sb_cloud_text_equal(const char *a, const char *b)
{
    u32 i = 0u;

    if ((a == 0) || (b == 0)) {
        return 0;
    }

    while ((a[i] != '\0') && (b[i] != '\0')) {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }

    return (a[i] == b[i]) && (b[i] == '\0');
}

int sb_cloud_url_is_https(const char *url)
{
    return sb_cloud_has_prefix(url, "https://");
}
