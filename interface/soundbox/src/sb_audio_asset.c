/*================================================================
 * Static QR UPI Soundbox - Audio Asset Paths and Validation
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_fs.h"
#include "sb_audio_asset.h"

static sb_status_t sb_path_append_char(char *path, u32 path_len, u32 *offset, char ch)
{
    if ((path == 0) || (offset == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if ((*offset + 1u) >= path_len) {
        return SB_STATUS_NO_MEMORY;
    }

    path[*offset] = ch;
    *offset = *offset + 1u;
    path[*offset] = '\0';
    return SB_STATUS_OK;
}

static sb_status_t sb_path_append_text(char *path, u32 path_len, u32 *offset, const char *text)
{
    u32 i;
    sb_status_t status;

    if (text == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    for (i = 0u; text[i] != '\0'; i++) {
        status = sb_path_append_char(path, path_len, offset, text[i]);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_path_append_u32(char *path, u32 path_len, u32 *offset, u32 value)
{
    char digits[10];
    u32 count = 0u;
    u32 i;

    if (value == 0u) {
        return sb_path_append_char(path, path_len, offset, '0');
    }

    while ((value != 0u) && (count < (u32)sizeof(digits))) {
        digits[count] = (char)('0' + (value % 10u));
        value = value / 10u;
        count++;
    }

    for (i = 0u; i < count; i++) {
        sb_status_t status = sb_path_append_char(path, path_len, offset, digits[count - 1u - i]);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    return SB_STATUS_OK;
}

static void sb_audio_copy_missing_path(char *dst, u32 dst_len, const char *src)
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

static sb_status_t sb_audio_begin_lang_path(sb_audio_language_t language,
                                            char *path,
                                            u32 path_len,
                                            u32 *offset)
{
    sb_status_t status;

    if ((path == 0) || (offset == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    path[0] = '\0';
    *offset = 0u;

    status = sb_path_append_text(path, path_len, offset, SB_AUDIO_ASSET_ROOT "/");
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_path_append_text(path, path_len, offset, sb_audio_language_code(language));
    if (status != SB_STATUS_OK) {
        return status;
    }

    return sb_path_append_char(path, path_len, offset, '/');
}

void sb_audio_script_init(sb_audio_script_t *script)
{
    u32 i;

    if (script == 0) {
        return;
    }

    script->count = 0u;
    for (i = 0u; i < SB_AUDIO_SCRIPT_MAX_ITEMS; i++) {
        script->items[i].path[0] = '\0';
    }
}

sb_status_t sb_audio_script_append_path(sb_audio_script_t *script, const char *path)
{
    u32 i;

    if ((script == 0) || (path == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (script->count >= SB_AUDIO_SCRIPT_MAX_ITEMS) {
        return SB_STATUS_NO_MEMORY;
    }

    for (i = 0u; (i + 1u < SB_AUDIO_PATH_LEN) && (path[i] != '\0'); i++) {
        script->items[script->count].path[i] = path[i];
    }
    script->items[script->count].path[i] = '\0';
    script->count++;
    return SB_STATUS_OK;
}

sb_status_t sb_audio_asset_build_prompt_path(sb_audio_language_t language,
                                             sb_audio_prompt_id_t prompt,
                                             char *path,
                                             u32 path_len)
{
    u32 offset;
    sb_status_t status;

    status = sb_audio_begin_lang_path(language, path, path_len, &offset);
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_path_append_text(path, path_len, &offset, "prompt_");
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_path_append_text(path, path_len, &offset, sb_audio_prompt_name(prompt));
    if (status != SB_STATUS_OK) {
        return status;
    }
    return sb_path_append_text(path, path_len, &offset, ".mp3");
}

sb_status_t sb_audio_asset_build_provider_path(sb_audio_language_t language,
                                               sb_audio_provider_t provider,
                                               char *path,
                                               u32 path_len)
{
    u32 offset;
    sb_status_t status;

    status = sb_audio_begin_lang_path(language, path, path_len, &offset);
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_path_append_text(path, path_len, &offset, "provider_");
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_path_append_text(path, path_len, &offset, sb_audio_provider_name(provider));
    if (status != SB_STATUS_OK) {
        return status;
    }
    return sb_path_append_text(path, path_len, &offset, ".mp3");
}

sb_status_t sb_audio_asset_build_amount_token_path(sb_audio_language_t language,
                                                   const sb_amount_token_t *token,
                                                   char *path,
                                                   u32 path_len)
{
    u32 offset;
    sb_status_t status;

    if (token == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_audio_begin_lang_path(language, path, path_len, &offset);
    if (status != SB_STATUS_OK) {
        return status;
    }

    switch (token->kind) {
    case SB_AMOUNT_TOKEN_NUMBER:
        status = sb_path_append_text(path, path_len, &offset, "num_");
        if (status != SB_STATUS_OK) { return status; }
        status = sb_path_append_u32(path, path_len, &offset, token->value);
        if (status != SB_STATUS_OK) { return status; }
        break;
    case SB_AMOUNT_TOKEN_HUNDRED:
        status = sb_path_append_text(path, path_len, &offset, "scale_hundred");
        break;
    case SB_AMOUNT_TOKEN_THOUSAND:
        status = sb_path_append_text(path, path_len, &offset, "scale_thousand");
        break;
    case SB_AMOUNT_TOKEN_LAKH:
        status = sb_path_append_text(path, path_len, &offset, "scale_lakh");
        break;
    case SB_AMOUNT_TOKEN_RUPEES:
        status = sb_path_append_text(path, path_len, &offset, "currency_rupees");
        break;
    case SB_AMOUNT_TOKEN_PAISE:
        status = sb_path_append_text(path, path_len, &offset, "currency_paise");
        break;
    case SB_AMOUNT_TOKEN_ONLY:
        status = sb_path_append_text(path, path_len, &offset, "currency_only");
        break;
    default:
        return SB_STATUS_INVALID_PARAM;
    }

    if (status != SB_STATUS_OK) {
        return status;
    }

    return sb_path_append_text(path, path_len, &offset, ".mp3");
}

int sb_audio_asset_exists(const char *path)
{
    if (path == 0) {
        return 0;
    }

    return (ql_access(path, 0u) == 0) ? 1 : 0;
}

sb_status_t sb_audio_asset_validate_script(const sb_audio_script_t *script,
                                           char *missing_path,
                                           u32 missing_path_len)
{
    u32 i;

    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    for (i = 0u; i < script->count; i++) {
        if (sb_audio_asset_exists(script->items[i].path) == 0) {
            sb_audio_copy_missing_path(missing_path, missing_path_len, script->items[i].path);
            return SB_STATUS_NOT_FOUND;
        }
    }

    if ((missing_path != 0) && (missing_path_len != 0u)) {
        missing_path[0] = '\0';
    }

    return SB_STATUS_OK;
}
