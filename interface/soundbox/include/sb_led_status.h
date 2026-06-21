/*================================================================
 * Static QR UPI Soundbox - Single LED Status Pattern Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_LED_STATUS_H
#define SB_LED_STATUS_H

#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SB_LED_STATUS_INTERNET_OK = 0,
    SB_LED_STATUS_NO_INTERNET,
    SB_LED_STATUS_NO_MQTT,
    SB_LED_STATUS_UNREGISTERED,
    SB_LED_STATUS_BATTERY_LOW,
    SB_LED_STATUS_VOLUME_MODE,
    SB_LED_STATUS_TRANSACTION_MODE,
    SB_LED_STATUS_ERROR
} sb_led_status_state_t;

sb_status_t sb_led_status_init(void);
sb_status_t sb_led_status_set(sb_led_status_state_t state);
sb_status_t sb_led_status_on_heartbeat(void);
const char *sb_led_status_name(sb_led_status_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* SB_LED_STATUS_H */
