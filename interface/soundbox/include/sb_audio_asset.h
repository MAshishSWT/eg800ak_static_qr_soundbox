/*================================================================
 * Static QR UPI Soundbox - Audio Asset Paths and Validation
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_ASSET_H
#define SB_AUDIO_ASSET_H

#include "sb_amount_tokenizer.h"
#include "sb_audio_types.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AUDIO_ASSET_ROOT         "U:/audio"

void sb_audio_script_init(sb_audio_script_t *script);
sb_status_t sb_audio_script_append_path(sb_audio_script_t *script, const char *path);
sb_status_t sb_audio_asset_build_common_path(const char *file, char *path, u32 path_len);
sb_status_t sb_audio_asset_build_alert_path(sb_audio_language_t language, const char *file, char *path, u32 path_len);
sb_status_t sb_audio_asset_build_audio_file_path(sb_audio_language_t language, const char *file, char *path, u32 path_len);
sb_status_t sb_audio_asset_build_prompt_path(sb_audio_language_t language,
                                             sb_audio_prompt_id_t prompt,
                                             char *path,
                                             u32 path_len);
sb_status_t sb_audio_asset_build_provider_path(sb_audio_language_t language,
                                               sb_audio_provider_t provider,
                                               char *path,
                                               u32 path_len);
sb_status_t sb_audio_asset_build_amount_token_path(sb_audio_language_t language,
                                                   const sb_amount_token_t *token,
                                                   char *path,
                                                   u32 path_len);
sb_status_t sb_audio_asset_validate_script(const sb_audio_script_t *script,
                                           char *missing_path,
                                           u32 missing_path_len);
int sb_audio_asset_exists(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_ASSET_H */
