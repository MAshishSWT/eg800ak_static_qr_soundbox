/*================================================================
 * Static QR UPI Soundbox - Minimal JSON Helpers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_json.h"
#include "sb_cloud_utils.h"

static const char *sb_json_find_key(const char *json, const char *key)
{
    u32 i;
    u32 j;

    if ((json == 0) || (key == 0)) {
        return 0;
    }

    for (i = 0u; json[i] != '\0'; i++) {
        if (json[i] != '"') {
            continue;
        }
        j = 0u;
        while ((key[j] != '\0') && (json[i + 1u + j] == key[j])) {
            j++;
        }
        if ((key[j] == '\0') && (json[i + 1u + j] == '"')) {
            return &json[i + 2u + j];
        }
    }

    return 0;
}

static const char *sb_json_skip_to_value(const char *p)
{
    if (p == 0) {
        return 0;
    }

    while ((*p == ' ') || (*p == '\t') || (*p == '\r') || (*p == '\n')) {
        p++;
    }
    if (*p != ':') {
        return 0;
    }
    p++;
    while ((*p == ' ') || (*p == '\t') || (*p == '\r') || (*p == '\n')) {
        p++;
    }
    return p;
}

sb_status_t sb_json_get_string(const char *json, const char *key, char *out, u32 out_len)
{
    const char *p;
    u32 i;

    if ((out == 0) || (out_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    out[0] = '\0';

    p = sb_json_skip_to_value(sb_json_find_key(json, key));
    if ((p == 0) || (*p != '"')) {
        return SB_STATUS_NOT_FOUND;
    }
    p++;

    for (i = 0u; (i + 1u < out_len) && (*p != '\0') && (*p != '"'); i++) {
        if ((*p == '\\') && (p[1] != '\0')) {
            p++;
        }
        out[i] = *p;
        p++;
    }
    out[i] = '\0';

    return (i > 0u) ? SB_STATUS_OK : SB_STATUS_NOT_FOUND;
}

sb_status_t sb_json_get_u64(const char *json, const char *key, u64 *out)
{
    const char *p;
    u64 value = 0u;
    int found = 0;

    if (out == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    *out = 0u;

    p = sb_json_skip_to_value(sb_json_find_key(json, key));
    if (p == 0) {
        return SB_STATUS_NOT_FOUND;
    }
    if (*p == '"') {
        p++;
    }
    while ((*p >= '0') && (*p <= '9')) {
        value = (value * 10u) + (u64)(*p - '0');
        found = 1;
        p++;
    }

    if (found == 0) {
        return SB_STATUS_NOT_FOUND;
    }
    *out = value;
    return SB_STATUS_OK;
}

static sb_status_t sb_json_parse_amount_decimal(const char *text, u64 *amount_paise)
{
    u64 rupees = 0u;
    u64 paise = 0u;
    u32 decimals = 0u;
    int found = 0;

    if ((text == 0) || (amount_paise == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    while ((*text >= '0') && (*text <= '9')) {
        rupees = (rupees * 10u) + (u64)(*text - '0');
        found = 1;
        text++;
    }
    if (*text == '.') {
        text++;
        while ((*text >= '0') && (*text <= '9') && (decimals < 2u)) {
            paise = (paise * 10u) + (u64)(*text - '0');
            decimals++;
            text++;
        }
    }
    if (decimals == 1u) {
        paise *= 10u;
    }
    if (found == 0) {
        return SB_STATUS_NOT_FOUND;
    }
    *amount_paise = (rupees * 100u) + paise;
    return SB_STATUS_OK;
}

sb_status_t sb_json_get_amount_paise(const char *json, u64 *amount_paise)
{
    char amount_text[24];
    sb_status_t status;

    status = sb_json_get_u64(json, "amount_paise", amount_paise);
    if (status == SB_STATUS_OK) {
        return status;
    }

    status = sb_json_get_string(json, "amount", amount_text, (u32)sizeof(amount_text));
    if (status == SB_STATUS_OK) {
        return sb_json_parse_amount_decimal(amount_text, amount_paise);
    }

    return SB_STATUS_NOT_FOUND;
}
