/*================================================================
 * Static QR UPI Soundbox - Key HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_board_kae8_sq1.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_hal_gpio.h"
#include "sb_hal_key.h"

static GPIO_PIN_NUMBER_E sb_key_to_gpio(sb_key_id_t key_id)
{
    switch (key_id) {
    case SB_KEY_VOLUME_UP:
        return SB_KAE8_KEY_VOLUME_UP_GPIO;
    case SB_KEY_VOLUME_DOWN:
        return SB_KAE8_KEY_VOLUME_DOWN_GPIO;
    case SB_KEY_MODE:
        return SB_KAE8_KEY_MODE_GPIO;
    default:
        return GPIO_PIN_NO_NOT_ASSIGNED;
    }
}

static void sb_key_post_edge(sb_key_id_t key_id)
{
    sb_key_state_t state;
    sb_event_t event;

    if (sb_hal_key_get_state(key_id, &state) != SB_STATUS_OK) {
        return;
    }

    sb_event_init(&event, SB_EVENT_KEY_EDGE, SB_EVENT_SOURCE_KEY);
    event.param_u32 = (u32)state.key_id;
    event.param_s32 = (s32)state.pressed;
    (void)sb_event_post(&event, QL_NO_WAIT);
    (void)sb_hal_gpio_enable_eint(sb_key_to_gpio(key_id), PIN_BOTH_EDGE);
}

static void sb_key_volume_up_irq(void)
{
    sb_key_post_edge(SB_KEY_VOLUME_UP);
}

static void sb_key_volume_down_irq(void)
{
    sb_key_post_edge(SB_KEY_VOLUME_DOWN);
}

static void sb_key_mode_irq(void)
{
    sb_key_post_edge(SB_KEY_MODE);
}

GPIO_PIN_NUMBER_E sb_hal_key_gpio(sb_key_id_t key_id)
{
    return sb_key_to_gpio(key_id);
}

sb_status_t sb_hal_key_get_state(sb_key_id_t key_id, sb_key_state_t *state)
{
    GPIO_PIN_NUMBER_E gpio;
    PIN_LEVEL_E level;

    if (state == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    gpio = sb_key_to_gpio(key_id);
    if (gpio == GPIO_PIN_NO_NOT_ASSIGNED) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (sb_hal_gpio_get(gpio, &level) != SB_STATUS_OK) {
        return SB_STATUS_ERROR;
    }

    state->key_id = key_id;
    state->level = level;
    state->pressed = (level == SB_KAE8_KEY_ACTIVE_LEVEL) ? 1 : 0;
    return SB_STATUS_OK;
}

static sb_status_t sb_hal_key_configure(sb_key_id_t key_id, sb_hal_gpio_irq_cb_t cb)
{
    GPIO_PIN_NUMBER_E gpio;
    sb_status_t status;

    gpio = sb_key_to_gpio(key_id);
    if ((gpio == GPIO_PIN_NO_NOT_ASSIGNED) || (cb == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_hal_gpio_input(gpio, SB_KAE8_KEY_IDLE_PULL);
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_hal_gpio_register_eint(gpio, PIN_BOTH_EDGE, SB_KAE8_KEY_IDLE_PULL, cb);
    if (status != SB_STATUS_OK) {
        return status;
    }

    return sb_hal_gpio_enable_eint(gpio, PIN_BOTH_EDGE);
}

sb_status_t sb_hal_key_init(void)
{
    sb_status_t status;

    status = sb_hal_key_configure(SB_KEY_VOLUME_UP, sb_key_volume_up_irq);
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_hal_key_configure(SB_KEY_VOLUME_DOWN, sb_key_volume_down_irq);
    if (status != SB_STATUS_OK) {
        return status;
    }

    return sb_hal_key_configure(SB_KEY_MODE, sb_key_mode_irq);
}
