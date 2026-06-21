/*================================================================
 * Static QR UPI Soundbox - EG800AK Audio HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_HAL_H
#define SB_AUDIO_HAL_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AUDIO_HAL_DEFAULT_TIMEOUT_MS       (12000u)
#define SB_AUDIO_HAL_READY_TIMEOUT_MS         (5000u)

sb_status_t sb_audio_hal_init(u32 volume_percent);
sb_status_t sb_audio_hal_set_volume_percent(u32 volume_percent);
sb_status_t sb_audio_hal_play_mp3_file(const char *path, u32 timeout_ms);
int sb_audio_hal_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_HAL_H */
