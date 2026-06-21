/*================================================================
 * Static QR UPI Soundbox - Audio Asset Paths and Validation
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_audio_asset.h"
#include "sb_audio_asset_store.h"
#include "sb_cloud_utils.h"

static const char *sb_audio_prompt_asset(sb_audio_prompt_id_t prompt, int *common)
{
    if (common != 0) {
        *common = 0;
    }
    switch (prompt) {
    case SB_AUDIO_PROMPT_POWER_ON:
        if (common != 0) { *common = 1; }
        return "start_tune.mp3";
    case SB_AUDIO_PROMPT_READY:
        return "internet.mp3";
    case SB_AUDIO_PROMPT_SETUP:
        return "unregistered_device.mp3";
    case SB_AUDIO_PROMPT_NO_SIM:
        return "no_SIM.mp3";
    case SB_AUDIO_PROMPT_NO_NETWORK:
    case SB_AUDIO_PROMPT_NO_INTERNET:
        return "no_internet.mp3";
    case SB_AUDIO_PROMPT_NO_MQTT:
        return "no_mqtt.mp3";
    case SB_AUDIO_PROMPT_BATTERY_LOW:
        return "battery_low.mp3";
    case SB_AUDIO_PROMPT_TRANSACTION_ERROR:
        if (common != 0) { *common = 1; }
        return "transaction_error.mp3";
    case SB_AUDIO_PROMPT_PAYMENT_RECEIVED:
        return "transaction_prefix.mp3";
    default:
        return "no_transactions.mp3";
    }
}

static sb_status_t sb_path_append(char *path, u32 path_len, const char *text)
{
    return sb_cloud_append_string(path, path_len, text);
}

static sb_status_t sb_lang_audio_files_path(sb_audio_language_t language,
                                            const char *file,
                                            char *path,
                                            u32 path_len)
{
    if ((file == 0) || (path == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    path[0] = '\0';
    if (sb_path_append(path, path_len, "audio/") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_path_append(path, path_len, sb_audio_language_asset_code(language)) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_path_append(path, path_len, "/audio_files/") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_path_append(path, path_len, file) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    return SB_STATUS_OK;
}

static sb_status_t sb_lang_alert_path(sb_audio_language_t language,
                                      const char *file,
                                      char *path,
                                      u32 path_len)
{
    if ((file == 0) || (path == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    path[0] = '\0';
    if (sb_path_append(path, path_len, "audio/") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_path_append(path, path_len, sb_audio_language_asset_code(language)) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_path_append(path, path_len, "/alerts/") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_path_append(path, path_len, file) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    return SB_STATUS_OK;
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
    if ((script == 0) || (path == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (script->count >= SB_AUDIO_SCRIPT_MAX_ITEMS) {
        return SB_STATUS_NO_MEMORY;
    }
    sb_cloud_copy_string(script->items[script->count].path, SB_AUDIO_PATH_LEN, path);
    script->count++;
    return SB_STATUS_OK;
}

sb_status_t sb_audio_asset_build_common_path(const char *file, char *path, u32 path_len)
{
    if ((file == 0) || (path == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    path[0] = '\0';
    if (sb_path_append(path, path_len, "audio/common/") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    return sb_path_append(path, path_len, file);
}

sb_status_t sb_audio_asset_build_alert_path(sb_audio_language_t language, const char *file, char *path, u32 path_len)
{
    return sb_lang_alert_path(language, file, path, path_len);
}

sb_status_t sb_audio_asset_build_audio_file_path(sb_audio_language_t language, const char *file, char *path, u32 path_len)
{
    return sb_lang_audio_files_path(language, file, path, path_len);
}

sb_status_t sb_audio_asset_build_prompt_path(sb_audio_language_t language,
                                             sb_audio_prompt_id_t prompt,
                                             char *path,
                                             u32 path_len)
{
    int common = 0;
    const char *asset = sb_audio_prompt_asset(prompt, &common);

    if ((path == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    path[0] = '\0';
    if (common != 0) {
        if (sb_path_append(path, path_len, "audio/common/") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
        return sb_path_append(path, path_len, asset);
    }
    return sb_lang_alert_path(language, asset, path, path_len);
}

sb_status_t sb_audio_asset_build_provider_path(sb_audio_language_t language,
                                               sb_audio_provider_t provider,
                                               char *path,
                                               u32 path_len)
{
    const char *asset;

    if ((path == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    switch (provider) {
    case SB_AUDIO_PROVIDER_GPAY:
        asset = "googlepay.mp3";
        break;
    case SB_AUDIO_PROVIDER_PAYTM:
    case SB_AUDIO_PROVIDER_PHONEPE:
    case SB_AUDIO_PROVIDER_BHIM:
    case SB_AUDIO_PROVIDER_OTHER:
    default:
        asset = "bank.mp3";
        break;
    }
    return sb_lang_audio_files_path(language, asset, path, path_len);
}

sb_status_t sb_audio_asset_build_amount_token_path(sb_audio_language_t language,
                                                   const sb_amount_token_t *token,
                                                   char *path,
                                                   u32 path_len)
{
    char name[20];

    if ((token == 0) || (path == 0) || (path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    name[0] = '\0';
    switch (token->kind) {
    case SB_AMOUNT_TOKEN_NUMBER:
        if (sb_cloud_append_u32(name, (u32)sizeof(name), token->value) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
        if (sb_cloud_append_string(name, (u32)sizeof(name), ".mp3") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
        break;
    case SB_AMOUNT_TOKEN_HUNDRED:
        sb_cloud_copy_string(name, (u32)sizeof(name), "hundred.mp3");
        break;
    case SB_AMOUNT_TOKEN_THOUSAND:
        sb_cloud_copy_string(name, (u32)sizeof(name), "thousand.mp3");
        break;
    case SB_AMOUNT_TOKEN_LAKH:
        sb_cloud_copy_string(name, (u32)sizeof(name), "lac.mp3");
        break;
    case SB_AMOUNT_TOKEN_CRORE:
        sb_cloud_copy_string(name, (u32)sizeof(name), "crore.mp3");
        break;
    case SB_AMOUNT_TOKEN_RUPEES:
        sb_cloud_copy_string(name, (u32)sizeof(name), "rupees.mp3");
        break;
    case SB_AMOUNT_TOKEN_PAISE:
        sb_cloud_copy_string(name, (u32)sizeof(name), "paise.mp3");
        break;
    case SB_AMOUNT_TOKEN_AND:
        sb_cloud_copy_string(name, (u32)sizeof(name), "and.mp3");
        break;
    default:
        return SB_STATUS_INVALID_PARAM;
    }
    return sb_lang_audio_files_path(language, name, path, path_len);
}

int sb_audio_asset_exists(const char *path)
{
    return sb_audio_asset_store_exists(path);
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
        if (sb_audio_asset_store_exists(script->items[i].path) == 0) {
            if ((missing_path != 0) && (missing_path_len != 0u)) {
                sb_cloud_copy_string(missing_path, missing_path_len, script->items[i].path);
            }
            return SB_STATUS_NOT_FOUND;
        }
    }
    if ((missing_path != 0) && (missing_path_len != 0u)) {
        missing_path[0] = '\0';
    }
    return SB_STATUS_OK;
}
