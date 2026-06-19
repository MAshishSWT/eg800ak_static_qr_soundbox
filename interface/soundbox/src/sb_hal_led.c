/*================================================================
 * Static QR UPI Soundbox - LED HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_board_kae8_sq1.h"
#include "sb_hal_gpio.h"
#include "sb_hal_led.h"

static int s_status_led_on = 0;

static PIN_LEVEL_E sb_led_level_from_state(int on)
{
    return (on != 0) ? SB_KAE8_STATUS_LED_ON_LEVEL : SB_KAE8_STATUS_LED_OFF_LEVEL;
}

sb_status_t sb_hal_led_init(void)
{
    s_status_led_on = 0;
    return sb_hal_gpio_output(SB_KAE8_STATUS_LED_GPIO, SB_KAE8_STATUS_LED_OFF_LEVEL);
}

sb_status_t sb_hal_led_set(sb_led_id_t led, int on)
{
    sb_status_t status;

    if (led != SB_LED_STATUS) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_hal_gpio_set(SB_KAE8_STATUS_LED_GPIO, sb_led_level_from_state(on));
    if (status == SB_STATUS_OK) {
        s_status_led_on = (on != 0) ? 1 : 0;
    }
    return status;
}

sb_status_t sb_hal_led_toggle(sb_led_id_t led)
{
    if (led != SB_LED_STATUS) {
        return SB_STATUS_INVALID_PARAM;
    }

    return sb_hal_led_set(led, (s_status_led_on == 0) ? 1 : 0);
}
