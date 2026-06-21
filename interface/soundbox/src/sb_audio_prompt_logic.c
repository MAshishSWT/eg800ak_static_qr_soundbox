/*================================================================
 * Static QR UPI Soundbox - Audio Business Prompt Logic
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_amount_tokenizer.h"
#include "sb_audio_asset.h"
#include "sb_audio_prompt_logic.h"

static int sb_lang_english(sb_audio_language_t language)
{
    return (language == SB_AUDIO_LANG_EN) ? 1 : 0;
}

static int sb_lang_suffix_used(sb_audio_language_t language)
{
    if ((language == SB_AUDIO_LANG_EN) || (language == SB_AUDIO_LANG_KN) ||
        (language == SB_AUDIO_LANG_ML) || (language == SB_AUDIO_LANG_TA) ||
        (language == SB_AUDIO_LANG_TE)) {
        return 0;
    }
    return 1;
}

static sb_status_t sb_logic_append_path_from_status(sb_audio_script_t *script, sb_status_t status,
                                                    const char *path)
{
    if (status != SB_STATUS_OK) {
        return status;
    }
    return sb_audio_script_append_path(script, path);
}

static sb_status_t sb_logic_append_audio_file(sb_audio_script_t *script,
                                              sb_audio_language_t language,
                                              const char *file)
{
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;

    status = sb_audio_asset_build_audio_file_path(language, file, path, (u32)sizeof(path));
    return sb_logic_append_path_from_status(script, status, path);
}

static sb_status_t sb_logic_append_alert(sb_audio_script_t *script,
                                         sb_audio_language_t language,
                                         const char *file)
{
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;

    status = sb_audio_asset_build_alert_path(language, file, path, (u32)sizeof(path));
    return sb_logic_append_path_from_status(script, status, path);
}

static sb_status_t sb_logic_append_common(sb_audio_script_t *script, const char *file)
{
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;

    status = sb_audio_asset_build_common_path(file, path, (u32)sizeof(path));
    return sb_logic_append_path_from_status(script, status, path);
}

static sb_status_t sb_logic_append_token(sb_audio_script_t *script,
                                         sb_audio_language_t language,
                                         sb_amount_token_kind_t kind,
                                         u32 value)
{
    sb_amount_token_t token;
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;

    token.kind = kind;
    token.value = value;
    status = sb_audio_asset_build_amount_token_path(language, &token, path, (u32)sizeof(path));
    return sb_logic_append_path_from_status(script, status, path);
}

static sb_status_t sb_logic_append_under_100(sb_audio_script_t *script, sb_audio_language_t language, u32 value)
{
    sb_status_t status;
    u32 tens;
    u32 ones;

    if (value == 0u) {
        return SB_STATUS_OK;
    }
    if (value <= 20u) {
        return sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_NUMBER, value);
    }
    tens = (value / 10u) * 10u;
    ones = value % 10u;
    status = sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_NUMBER, tens);
    if ((status == SB_STATUS_OK) && (ones != 0u)) {
        status = sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_NUMBER, ones);
    }
    return status;
}

static sb_status_t sb_logic_append_under_1000(sb_audio_script_t *script, sb_audio_language_t language, u32 value)
{
    sb_status_t status;
    u32 hundreds = value / 100u;
    u32 rem = value % 100u;

    if (hundreds != 0u) {
        status = sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_NUMBER, hundreds);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_HUNDRED, 0u);
        if (status != SB_STATUS_OK) { return status; }
    }
    return sb_logic_append_under_100(script, language, rem);
}

static sb_status_t sb_logic_append_number(sb_audio_script_t *script, sb_audio_language_t language, u64 value)
{
    sb_status_t status;
    u32 crore;
    u32 lakh;
    u32 thousand;
    u32 rem;

    if (value == 0ull) {
        return sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_NUMBER, 0u);
    }
    if (value > 999999999ull) {
        return SB_STATUS_UNSUPPORTED;
    }
    crore = (u32)(value / 10000000ull);
    rem = (u32)(value % 10000000ull);
    lakh = rem / 100000u;
    rem = rem % 100000u;
    thousand = rem / 1000u;
    rem = rem % 1000u;

    if (crore != 0u) {
        status = sb_logic_append_under_100(script, language, crore);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_CRORE, 0u);
        if (status != SB_STATUS_OK) { return status; }
    }
    if (lakh != 0u) {
        status = sb_logic_append_under_100(script, language, lakh);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_LAKH, 0u);
        if (status != SB_STATUS_OK) { return status; }
    }
    if (thousand != 0u) {
        status = sb_logic_append_under_100(script, language, thousand);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_token(script, language, SB_AMOUNT_TOKEN_THOUSAND, 0u);
        if (status != SB_STATUS_OK) { return status; }
    }
    return sb_logic_append_under_1000(script, language, rem);
}

static sb_status_t sb_logic_append_provider(sb_audio_script_t *script,
                                            sb_audio_language_t language,
                                            sb_audio_provider_t provider)
{
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;

    status = sb_audio_asset_build_provider_path(language, provider, path, (u32)sizeof(path));
    return sb_logic_append_path_from_status(script, status, path);
}

sb_status_t sb_audio_prompt_logic_build_common(const char *file, sb_audio_script_t *script)
{
    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    sb_audio_script_init(script);
    return sb_logic_append_common(script, file);
}

sb_status_t sb_audio_prompt_logic_build_alert(sb_audio_language_t language, const char *file, sb_audio_script_t *script)
{
    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    sb_audio_script_init(script);
    return sb_logic_append_alert(script, language, file);
}

sb_status_t sb_audio_prompt_logic_build_health(sb_audio_language_t language,
                                                sb_audio_health_kind_t kind,
                                                u32 percent,
                                                sb_audio_script_t *script)
{
    sb_status_t status;

    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    sb_audio_script_init(script);
    status = sb_logic_append_alert(script, language,
                                   (kind == SB_AUDIO_HEALTH_BATTERY) ? "battery_prefix.mp3" : "internet_prefix.mp3");
    if (status != SB_STATUS_OK) { return status; }
    status = sb_logic_append_number(script, language, (u64)percent);
    if (status != SB_STATUS_OK) { return status; }
    return sb_logic_append_alert(script, language, "percent.mp3");
}

static sb_status_t sb_logic_append_rupees_and_paise(sb_audio_script_t *script,
                                                    sb_audio_language_t language,
                                                    u64 amount_paise,
                                                    int with_paise)
{
    sb_status_t status;
    u64 rupees = amount_paise / 100ull;
    u32 paise = (u32)(amount_paise % 100ull);

    status = sb_logic_append_number(script, language, rupees);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_logic_append_audio_file(script, language, "rupees.mp3");
    if (status != SB_STATUS_OK) { return status; }
    if ((with_paise != 0) && (paise != 0u)) {
        status = sb_logic_append_audio_file(script, language, "and.mp3");
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_number(script, language, (u64)paise);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_audio_file(script, language, "paise.mp3");
        if (status != SB_STATUS_OK) { return status; }
    }
    return SB_STATUS_OK;
}

sb_status_t sb_audio_prompt_logic_build_transaction(sb_audio_language_t language,
                                                     sb_audio_provider_t provider,
                                                     u64 amount_paise,
                                                     u32 options,
                                                     sb_audio_script_t *script)
{
    sb_status_t status;
    int with_paise;

    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    sb_audio_script_init(script);
    with_paise = ((amount_paise % 100ull) != 0ull) ? 1 : 0;

    status = sb_logic_append_audio_file(script, language, "transaction_prefix.mp3");
    if (status != SB_STATUS_OK) { return status; }

    if (sb_lang_english(language) != 0) {
        status = sb_logic_append_rupees_and_paise(script, language, amount_paise, with_paise);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_provider(script, language, provider);
        if (status != SB_STATUS_OK) { return status; }
    } else {
        status = sb_logic_append_provider(script, language, provider);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_rupees_and_paise(script, language, amount_paise, with_paise);
        if (status != SB_STATUS_OK) { return status; }
        if (sb_lang_suffix_used(language) != 0) {
            status = sb_logic_append_audio_file(script, language, "suffix.mp3");
            if (status != SB_STATUS_OK) { return status; }
        }
    }

    status = sb_logic_append_audio_file(script, language, "thankyou.mp3");
    if (status != SB_STATUS_OK) { return status; }

    /* advert.mp3 is referenced by the business workbook but is not present in
     * the supplied asset pack. The advertisement token is optional by product
     * rule, so it is deliberately skipped instead of failing transaction audio.
     */
    (void)options;
    return SB_STATUS_OK;
}

sb_status_t sb_audio_prompt_logic_build_last_transaction(sb_audio_language_t language,
                                                          const sb_transaction_record_t *record,
                                                          sb_audio_script_t *script)
{
    sb_status_t status;
    int with_paise;

    if ((record == 0) || (script == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    sb_audio_script_init(script);
    with_paise = ((record->amount_paise % 100ull) != 0ull) ? 1 : 0;

    status = sb_logic_append_audio_file(script, language, "last_transaction_prefix.mp3");
    if (status != SB_STATUS_OK) { return status; }
    status = sb_logic_append_number(script, language, 1ull);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_logic_append_audio_file(script, language, "has.mp3");
    if (status != SB_STATUS_OK) { return status; }
    status = sb_logic_append_rupees_and_paise(script, language, record->amount_paise, with_paise);
    if (status != SB_STATUS_OK) { return status; }
    if (sb_lang_suffix_used(language) != 0) {
        status = sb_logic_append_audio_file(script, language, "is.mp3");
    }
    return status;
}

sb_status_t sb_audio_prompt_logic_build_daily_summary(sb_audio_language_t language,
                                                       const sb_daily_summary_t *summary,
                                                       sb_audio_script_t *script)
{
    sb_status_t status;
    int with_paise;

    if ((summary == 0) || (script == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (summary->count == 0u) {
        return sb_audio_prompt_logic_build_alert(language, "no_transactions.mp3", script);
    }

    sb_audio_script_init(script);
    with_paise = ((summary->total_paise % 100ull) != 0ull) ? 1 : 0;
    status = sb_logic_append_audio_file(script, language, "summary_prefix.mp3");
    if (status != SB_STATUS_OK) { return status; }

    if (sb_lang_english(language) != 0) {
        status = sb_logic_append_audio_file(script, language, "is.mp3");
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_rupees_and_paise(script, language, summary->total_paise, with_paise);
        if (status != SB_STATUS_OK) { return status; }
        status = sb_logic_append_number(script, language, (u64)summary->count);
        if (status != SB_STATUS_OK) { return status; }
        return sb_logic_append_audio_file(script, language, (summary->count == 1u) ? "single_txn.mp3" : "transactions.mp3");
    }

    status = sb_logic_append_number(script, language, (u64)summary->count);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_logic_append_audio_file(script, language, (summary->count == 1u) ? "single_txn.mp3" : "transactions.mp3");
    if (status != SB_STATUS_OK) { return status; }
    status = sb_logic_append_rupees_and_paise(script, language, summary->total_paise, with_paise);
    if (status != SB_STATUS_OK) { return status; }
    if (sb_lang_suffix_used(language) != 0) {
        return sb_logic_append_audio_file(script, language, "suffix.mp3");
    }
    return SB_STATUS_OK;
}
