/*================================================================
 * Static QR UPI Soundbox - Audio Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_SERVICE_H
#define SB_AUDIO_SERVICE_H

#include "ql_type.h"
#include "sb_audio_types.h"
#include "sb_error.h"

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

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_SERVICE_H */
