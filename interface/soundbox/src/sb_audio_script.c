/*================================================================
 * Static QR UPI Soundbox - Audio Script Builder
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_amount_tokenizer.h"
#include "sb_audio_asset.h"
#include "sb_audio_script.h"

static sb_status_t sb_audio_script_append_prompt(sb_audio_script_t *script,
                                                 sb_audio_language_t language,
                                                 sb_audio_prompt_id_t prompt)
{
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;

    status = sb_audio_asset_build_prompt_path(language, prompt, path, (u32)sizeof(path));
    if (status != SB_STATUS_OK) {
        return status;
    }

    return sb_audio_script_append_path(script, path);
}

sb_status_t sb_audio_script_build_status(sb_audio_language_t language,
                                         sb_audio_prompt_id_t prompt,
                                         sb_audio_script_t *script)
{
    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    sb_audio_script_init(script);
    return sb_audio_script_append_prompt(script, language, prompt);
}

sb_status_t sb_audio_script_build_amount_received(sb_audio_language_t language,
                                                  sb_audio_provider_t provider,
                                                  u64 amount_paise,
                                                  sb_audio_script_t *script)
{
    sb_amount_token_list_t token_list;
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;
    u32 i;

    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    sb_audio_script_init(script);

    if ((provider > SB_AUDIO_PROVIDER_OTHER) && (provider < SB_AUDIO_PROVIDER_COUNT)) {
        status = sb_audio_asset_build_provider_path(language, provider, path, (u32)sizeof(path));
        if (status != SB_STATUS_OK) {
            return status;
        }
        status = sb_audio_script_append_path(script, path);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    status = sb_audio_script_append_prompt(script, language, SB_AUDIO_PROMPT_PAYMENT_RECEIVED);
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_amount_tokenize_paise(amount_paise, &token_list);
    if (status != SB_STATUS_OK) {
        return status;
    }

    for (i = 0u; i < token_list.count; i++) {
        status = sb_audio_asset_build_amount_token_path(language,
                                                        &token_list.tokens[i],
                                                        path,
                                                        (u32)sizeof(path));
        if (status != SB_STATUS_OK) {
            return status;
        }
        status = sb_audio_script_append_path(script, path);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    return SB_STATUS_OK;
}
