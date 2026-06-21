/*================================================================
 * Static QR UPI Soundbox - Audio Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_SERVICE_H
#define SB_AUDIO_SERVICE_H

#include "ql_type.h"
#include "sb_audio_types.h"
#include "sb_error.h"
#include "sb_transaction_ledger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AUDIO_SERVICE_TASK_STACK_BYTES     (12u * 1024u)
#define SB_AUDIO_SERVICE_TASK_PRIORITY        (14u)
#define SB_AUDIO_SERVICE_QUEUE_DEPTH          (8u)

sb_status_t sb_audio_service_init(sb_audio_language_t language, u32 volume_percent);
sb_status_t sb_audio_service_play_prompt(sb_audio_language_t language, sb_audio_prompt_id_t prompt);
sb_status_t sb_audio_service_play_amount(sb_audio_language_t language,
                                          sb_audio_provider_t provider,
                                          u64 amount_paise);
sb_status_t sb_audio_service_set_volume(u32 volume_percent);
sb_status_t sb_audio_service_play_common(const char *common_file);
sb_status_t sb_audio_service_play_alert(sb_audio_language_t language, const char *alert_file);
sb_status_t sb_audio_service_play_health(sb_audio_language_t language, int battery, u32 percent);
sb_status_t sb_audio_service_play_last_transaction(sb_audio_language_t language, const sb_transaction_record_t *record);
sb_status_t sb_audio_service_play_daily_summary(sb_audio_language_t language, const sb_daily_summary_t *summary);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_SERVICE_H */
