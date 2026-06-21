/*================================================================
 * Static QR UPI Soundbox - Audio Script Builder
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_SCRIPT_H
#define SB_AUDIO_SCRIPT_H

#include "ql_type.h"
#include "sb_audio_types.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

sb_status_t sb_audio_script_build_status(sb_audio_language_t language,
                                         sb_audio_prompt_id_t prompt,
                                         sb_audio_script_t *script);
sb_status_t sb_audio_script_build_amount_received(sb_audio_language_t language,
                                                  sb_audio_provider_t provider,
                                                  u64 amount_paise,
                                                  sb_audio_script_t *script);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_SCRIPT_H */
