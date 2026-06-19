/*================================================================
 * Static QR UPI Soundbox - LED HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_HAL_LED_H
#define SB_HAL_LED_H

#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SB_LED_STATUS = 0
} sb_led_id_t;

sb_status_t sb_hal_led_init(void);
sb_status_t sb_hal_led_set(sb_led_id_t led, int on);
sb_status_t sb_hal_led_toggle(sb_led_id_t led);

#ifdef __cplusplus
}
#endif

#endif /* SB_HAL_LED_H */
