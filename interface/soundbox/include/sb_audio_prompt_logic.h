/*================================================================
 * Static QR UPI Soundbox - Audio Business Prompt Logic
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_PROMPT_LOGIC_H
#define SB_AUDIO_PROMPT_LOGIC_H

#include "ql_type.h"
#include "sb_audio_types.h"
#include "sb_error.h"
#include "sb_transaction_ledger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AUDIO_PROMPT_LOGIC_OPTION_ADVERT (1u)

typedef enum {
    SB_AUDIO_HEALTH_INTERNET = 0,
    SB_AUDIO_HEALTH_BATTERY
} sb_audio_health_kind_t;

sb_status_t sb_audio_prompt_logic_build_common(const char *file, sb_audio_script_t *script);
sb_status_t sb_audio_prompt_logic_build_alert(sb_audio_language_t language, const char *file, sb_audio_script_t *script);
sb_status_t sb_audio_prompt_logic_build_health(sb_audio_language_t language,
                                                sb_audio_health_kind_t kind,
                                                u32 percent,
                                                sb_audio_script_t *script);
sb_status_t sb_audio_prompt_logic_build_transaction_fallback(sb_audio_language_t language,
                                                              sb_audio_provider_t provider,
                                                              sb_audio_script_t *script);

sb_status_t sb_audio_prompt_logic_build_transaction(sb_audio_language_t language,
                                                     sb_audio_provider_t provider,
                                                     u64 amount_paise,
                                                     u32 options,
                                                     sb_audio_script_t *script);
sb_status_t sb_audio_prompt_logic_build_last_transaction(sb_audio_language_t language,
                                                          const sb_transaction_record_t *record,
                                                          sb_audio_script_t *script);
sb_status_t sb_audio_prompt_logic_build_daily_summary(sb_audio_language_t language,
                                                       const sb_daily_summary_t *summary,
                                                       sb_audio_script_t *script);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_PROMPT_LOGIC_H */
