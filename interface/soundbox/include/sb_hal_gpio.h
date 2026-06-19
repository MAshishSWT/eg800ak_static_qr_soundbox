/*================================================================
 * Static QR UPI Soundbox - GPIO HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_HAL_GPIO_H
#define SB_HAL_GPIO_H

#include "ql_gpio.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sb_hal_gpio_irq_cb_t)(void);

sb_status_t sb_hal_gpio_input(GPIO_PIN_NUMBER_E pin, PIN_PULL_E pull);
sb_status_t sb_hal_gpio_output(GPIO_PIN_NUMBER_E pin, PIN_LEVEL_E level);
sb_status_t sb_hal_gpio_set(GPIO_PIN_NUMBER_E pin, PIN_LEVEL_E level);
sb_status_t sb_hal_gpio_get(GPIO_PIN_NUMBER_E pin, PIN_LEVEL_E *level);
sb_status_t sb_hal_gpio_register_eint(GPIO_PIN_NUMBER_E pin, PIN_EDGE_E edge, PIN_PULL_E pull, sb_hal_gpio_irq_cb_t cb);
sb_status_t sb_hal_gpio_enable_eint(GPIO_PIN_NUMBER_E pin, PIN_EDGE_E edge);
sb_status_t sb_hal_gpio_disable_eint(GPIO_PIN_NUMBER_E pin);

#ifdef __cplusplus
}
#endif

#endif /* SB_HAL_GPIO_H */
