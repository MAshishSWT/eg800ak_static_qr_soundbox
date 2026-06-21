/*================================================================
 * Static QR UPI Soundbox - Single LED Status Pattern Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * KAE8_SQ1 has one firmware-controlled USER_LED_1. The product
 * README describes RGB/three LED states, so this module maps semantic
 * states to one-LED blink patterns without adding RGB dependencies.
 *================================================================*/
#include "ql_rtos.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_led_status.h"
#include "sb_log.h"

#define SB_LED_STATUS_MODULE_NAME "led_status"

static sb_led_status_state_t s_led_state = SB_LED_STATUS_INTERNET_OK;
static u32 s_led_phase = 0u;
static u32 s_led_pulse_left = 0u;

const char *sb_led_status_name(sb_led_status_state_t state)
{
    switch (state) {
    case SB_LED_STATUS_INTERNET_OK:
        return "internet_ok";
    case SB_LED_STATUS_NO_INTERNET:
        return "no_internet";
    case SB_LED_STATUS_NO_MQTT:
        return "no_mqtt";
    case SB_LED_STATUS_UNREGISTERED:
        return "unregistered";
    case SB_LED_STATUS_BATTERY_LOW:
        return "battery_low";
    case SB_LED_STATUS_VOLUME_MODE:
        return "volume_mode";
    case SB_LED_STATUS_TRANSACTION_MODE:
        return "transaction_mode";
    case SB_LED_STATUS_ERROR:
    default:
        return "error";
    }
}

sb_status_t sb_led_status_init(void)
{
    s_led_state = SB_LED_STATUS_INTERNET_OK;
    s_led_phase = 0u;
    s_led_pulse_left = 0u;
    return sb_bsp_board_set_status_led(0);
}

sb_status_t sb_led_status_set(sb_led_status_state_t state)
{
    if ((state < SB_LED_STATUS_INTERNET_OK) || (state > SB_LED_STATUS_ERROR)) {
        return SB_STATUS_INVALID_PARAM;
    }

    s_led_state = state;
    s_led_phase = 0u;
    if (state == SB_LED_STATUS_VOLUME_MODE) {
        s_led_pulse_left = 2u;
    } else if (state == SB_LED_STATUS_TRANSACTION_MODE) {
        s_led_pulse_left = 4u;
    } else {
        s_led_pulse_left = 0u;
    }

    SB_LOGI(SB_LED_STATUS_MODULE_NAME, "state=%s", sb_led_status_name(state));
    return SB_STATUS_OK;
}

static int sb_led_status_pattern_value(void)
{
    u32 phase;

    if (s_led_pulse_left != 0u) {
        s_led_pulse_left--;
        return ((s_led_pulse_left & 1u) == 0u) ? 1 : 0;
    }

    phase = s_led_phase % 8u;
    switch (s_led_state) {
    case SB_LED_STATUS_INTERNET_OK:
        return 1;
    case SB_LED_STATUS_NO_INTERNET:
        return ((s_led_phase & 1u) == 0u) ? 1 : 0;
    case SB_LED_STATUS_NO_MQTT:
        return 1;
    case SB_LED_STATUS_UNREGISTERED:
        return ((phase == 0u) || (phase == 2u)) ? 1 : 0;
    case SB_LED_STATUS_BATTERY_LOW:
        return ((phase == 0u) || (phase == 2u) || (phase == 4u)) ? 1 : 0;
    case SB_LED_STATUS_ERROR:
        return ((s_led_phase & 1u) == 0u) ? 1 : 0;
    case SB_LED_STATUS_VOLUME_MODE:
    case SB_LED_STATUS_TRANSACTION_MODE:
    default:
        return 0;
    }
}

sb_status_t sb_led_status_on_heartbeat(void)
{
    int on;

    on = sb_led_status_pattern_value();
    s_led_phase++;
    return sb_bsp_board_set_status_led(on);
}
