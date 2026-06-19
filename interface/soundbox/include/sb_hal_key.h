/*================================================================
 * Static QR UPI Soundbox - Key HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_HAL_KEY_H
#define SB_HAL_KEY_H

#include "ql_gpio.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SB_KEY_VOLUME_UP = 1,
    SB_KEY_VOLUME_DOWN = 2,
    SB_KEY_MODE = 3
} sb_key_id_t;

typedef struct {
    sb_key_id_t key_id;
    PIN_LEVEL_E level;
    int pressed;
} sb_key_state_t;

sb_status_t sb_hal_key_init(void);
sb_status_t sb_hal_key_get_state(sb_key_id_t key_id, sb_key_state_t *state);
GPIO_PIN_NUMBER_E sb_hal_key_gpio(sb_key_id_t key_id);

#ifdef __cplusplus
}
#endif

#endif /* SB_HAL_KEY_H */
