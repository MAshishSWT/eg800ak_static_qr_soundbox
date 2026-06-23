/*================================================================
 * Static QR UPI Soundbox - Single LED Status Pattern Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * Correct one-LED business mapping:
 * - internet_ok       : solid ON
 * - no_internet       : slow blink
 * - no_mqtt           : fast blink
 * - unregistered      : double blink
 * - battery_low       : triple blink
 * - volume_mode       : one acknowledgement blink, then previous state
 * - transaction_mode  : two acknowledgement blinks, then previous state
 * - error             : rapid blink
 *================================================================*/
#include "ql_rtos.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_led_status.h"
#include "sb_log.h"

#define SB_LED_STATUS_MODULE_NAME "led_status"

static sb_led_status_state_t s_led_state = SB_LED_STATUS_NO_INTERNET;
static sb_led_status_state_t s_led_base_state = SB_LED_STATUS_NO_INTERNET;
static u32 s_led_phase = 0u;
static u32 s_led_ack_toggles_left = 0u;

const char *sb_led_status_name(sb_led_status_state_t state)
{
    switch (state) {
    case SB_LED_STATUS_INTERNET_OK: return "internet_ok";
    case SB_LED_STATUS_NO_INTERNET: return "no_internet";
    case SB_LED_STATUS_NO_MQTT: return "no_mqtt";
    case SB_LED_STATUS_UNREGISTERED: return "unregistered";
    case SB_LED_STATUS_BATTERY_LOW: return "battery_low";
    case SB_LED_STATUS_VOLUME_MODE: return "volume_mode";
    case SB_LED_STATUS_TRANSACTION_MODE: return "transaction_mode";
    case SB_LED_STATUS_ERROR:
    default: return "error";
    }
}

sb_status_t sb_led_status_init(void)
{
    s_led_state = SB_LED_STATUS_NO_INTERNET;
    s_led_base_state = SB_LED_STATUS_NO_INTERNET;
    s_led_phase = 0u;
    s_led_ack_toggles_left = 0u;
    return sb_bsp_board_set_status_led(0);
}

sb_status_t sb_led_status_set(sb_led_status_state_t state)
{
    if ((state < SB_LED_STATUS_INTERNET_OK) || (state > SB_LED_STATUS_ERROR)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if ((state == SB_LED_STATUS_VOLUME_MODE) || (state == SB_LED_STATUS_TRANSACTION_MODE)) {
        s_led_state = state;
        s_led_ack_toggles_left = (state == SB_LED_STATUS_VOLUME_MODE) ? 2u : 4u;
    } else {
        s_led_state = state;
        s_led_base_state = state;
        s_led_ack_toggles_left = 0u;
    }
    s_led_phase = 0u;
    SB_LOGI(SB_LED_STATUS_MODULE_NAME, "state=%s", sb_led_status_name(state));
    return SB_STATUS_OK;
}

static int sb_led_base_pattern(sb_led_status_state_t state, u32 phase)
{
    u32 p16 = phase % 16u;
    u32 p8 = phase % 8u;

    switch (state) {
    case SB_LED_STATUS_INTERNET_OK:
        return 1;
    case SB_LED_STATUS_NO_INTERNET:
        return (p16 < 8u) ? 1 : 0;
    case SB_LED_STATUS_NO_MQTT:
        return ((phase & 1u) == 0u) ? 1 : 0;
    case SB_LED_STATUS_UNREGISTERED:
        return ((p16 == 0u) || (p16 == 2u)) ? 1 : 0;
    case SB_LED_STATUS_BATTERY_LOW:
        return ((p16 == 0u) || (p16 == 2u) || (p16 == 4u)) ? 1 : 0;
    case SB_LED_STATUS_ERROR:
        return ((p8 & 1u) == 0u) ? 1 : 0;
    case SB_LED_STATUS_VOLUME_MODE:
    case SB_LED_STATUS_TRANSACTION_MODE:
    default:
        return 0;
    }
}

sb_status_t sb_led_status_on_heartbeat(void)
{
    int on;

    if (s_led_ack_toggles_left != 0u) {
        on = ((s_led_ack_toggles_left & 1u) == 0u) ? 1 : 0;
        s_led_ack_toggles_left--;
        if (s_led_ack_toggles_left == 0u) {
            s_led_state = s_led_base_state;
            s_led_phase = 0u;
        }
    } else {
        on = sb_led_base_pattern(s_led_state, s_led_phase);
        s_led_phase++;
    }
    return sb_bsp_board_set_status_led(on);
}
