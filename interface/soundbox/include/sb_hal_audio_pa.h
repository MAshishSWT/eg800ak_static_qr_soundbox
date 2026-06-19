/*================================================================
 * Static QR UPI Soundbox - Speaker Amplifier HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_HAL_AUDIO_PA_H
#define SB_HAL_AUDIO_PA_H

#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

sb_status_t sb_hal_audio_pa_init(void);
sb_status_t sb_hal_audio_pa_set_enabled(int enabled);
int sb_hal_audio_pa_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_HAL_AUDIO_PA_H */
