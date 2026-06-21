/*================================================================
 * Static QR UPI Soundbox - Audio Types
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_audio_types.h"

static const char *const s_language_codes[SB_AUDIO_LANG_COUNT] = {
    "en", "hi", "mr", "gu", "bn", "kn", "ml", "ta", "te", "pa"
};

static const char *const s_provider_names[SB_AUDIO_PROVIDER_COUNT] = {
    "other", "paytm", "phonepe", "gpay", "bhim"
};

static const char *const s_language_asset_codes[SB_AUDIO_LANG_COUNT] = {
    "en", "hi", "ma", "gu", "bn", "kn", "ml", "ta", "tl", "pa"
};

static int sb_audio_code_equals(const char *a, const char *b)
{
    u32 i;

    if ((a == 0) || (b == 0)) {
        return 0;
    }

    for (i = 0u; i < SB_AUDIO_LANG_CODE_LEN; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
        if ((a[i] == '\0') && (b[i] == '\0')) {
            return 1;
        }
    }

    return 1;
}

static const char *const s_prompt_names[SB_AUDIO_PROMPT_COUNT] = {
    "power_on",
    "ready",
    "setup",
    "no_sim",
    "no_network",
    "no_internet",
    "no_mqtt",
    "battery_low",
    "transaction_error",
    "payment_received"
};

const char *sb_audio_language_code(sb_audio_language_t language)
{
    if ((language < 0) || (language >= SB_AUDIO_LANG_COUNT)) {
        return s_language_codes[SB_AUDIO_LANG_EN];
    }
    return s_language_codes[language];
}

const char *sb_audio_language_asset_code(sb_audio_language_t language)
{
    if ((language < 0) || (language >= SB_AUDIO_LANG_COUNT)) {
        return s_language_asset_codes[SB_AUDIO_LANG_EN];
    }
    return s_language_asset_codes[language];
}

const char *sb_audio_provider_name(sb_audio_provider_t provider)
{
    if ((provider < 0) || (provider >= SB_AUDIO_PROVIDER_COUNT)) {
        return s_provider_names[SB_AUDIO_PROVIDER_OTHER];
    }
    return s_provider_names[provider];
}

const char *sb_audio_prompt_name(sb_audio_prompt_id_t prompt)
{
    if ((prompt < 0) || (prompt >= SB_AUDIO_PROMPT_COUNT)) {
        return "unknown";
    }
    return s_prompt_names[prompt];
}

sb_audio_language_t sb_audio_language_from_code(const char *code)
{
    int i;

    if (code == 0) {
        return SB_AUDIO_LANG_EN;
    }

    for (i = 0; i < (int)SB_AUDIO_LANG_COUNT; i++) {
        if ((sb_audio_code_equals(code, s_language_codes[i]) != 0) ||
            (sb_audio_code_equals(code, s_language_asset_codes[i]) != 0)) {
            return (sb_audio_language_t)i;
        }
    }

    return SB_AUDIO_LANG_EN;
}
