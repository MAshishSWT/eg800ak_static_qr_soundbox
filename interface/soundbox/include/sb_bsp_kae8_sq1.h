/*================================================================
 * Static QR UPI Soundbox - KAE8_SQ1 BSP
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_BSP_KAE8_SQ1_H
#define SB_BSP_KAE8_SQ1_H

#include "sb_error.h"
#include "sb_hal_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

sb_status_t sb_bsp_board_init(void);
sb_status_t sb_bsp_board_post_status(void);
sb_status_t sb_bsp_board_set_status_led(int on);
sb_status_t sb_bsp_board_toggle_status_led(void);
sb_status_t sb_bsp_board_set_speaker_amp(int enabled);
sb_status_t sb_bsp_board_read_battery(sb_battery_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* SB_BSP_KAE8_SQ1_H */
