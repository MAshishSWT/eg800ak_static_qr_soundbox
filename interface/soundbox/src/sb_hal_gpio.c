/*================================================================
 * Static QR UPI Soundbox - GPIO HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_hal_gpio.h"

sb_status_t sb_hal_gpio_input(GPIO_PIN_NUMBER_E pin, PIN_PULL_E pull)
{
    if (ql_gpio_init(pin, PIN_DIRECTION_IN, pull, PIN_LEVEL_HIGH) != 0) {
        return SB_STATUS_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_hal_gpio_output(GPIO_PIN_NUMBER_E pin, PIN_LEVEL_E level)
{
    if (ql_gpio_init(pin, PIN_DIRECTION_OUT, PIN_PULL_DISABLE, level) != 0) {
        return SB_STATUS_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_hal_gpio_set(GPIO_PIN_NUMBER_E pin, PIN_LEVEL_E level)
{
    if (ql_gpio_set_level(pin, level) != 0) {
        return SB_STATUS_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_hal_gpio_get(GPIO_PIN_NUMBER_E pin, PIN_LEVEL_E *level)
{
    if (level == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_gpio_get_level(pin, level) != 0) {
        return SB_STATUS_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_hal_gpio_register_eint(GPIO_PIN_NUMBER_E pin, PIN_EDGE_E edge, PIN_PULL_E pull, sb_hal_gpio_irq_cb_t cb)
{
    if (cb == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_eint_register(pin, edge, pull, (void *)cb, 0) != 0) {
        return SB_STATUS_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_hal_gpio_enable_eint(GPIO_PIN_NUMBER_E pin, PIN_EDGE_E edge)
{
    if (ql_eint_enable(pin, edge) != 0) {
        return SB_STATUS_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_hal_gpio_disable_eint(GPIO_PIN_NUMBER_E pin)
{
    if (ql_eint_disable(pin) != 0) {
        return SB_STATUS_ERROR;
    }
    return SB_STATUS_OK;
}
