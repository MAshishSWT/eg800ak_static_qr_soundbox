/*================================================================
 * Static QR UPI Soundbox - KAE8_SQ1 Board Mapping
 * Target: Quectel EG800AK-CN QuecOpen SDK
 * Source: KAE8_SQ1_260611_Sch_01 schematic, sheets 1-3
 * Source: Quectel EG800AK Series QuecOpen GPIO Configuration V1.1
 *
 * This header records schematic-to-QuecOpen-GPIO constants for the soundbox BSP.
 * Use GPIO[x] numbers from the Quectel GPIO configuration sheet, not physical pin numbers.
 *================================================================*/
#ifndef SB_BOARD_KAE8_SQ1_H
#define SB_BOARD_KAE8_SQ1_H

#include "ql_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ADC */
#define SB_KAE8_ADC_BATTERY_CHANNEL              (0u) /* BATT_VTG_SENS -> EG800AK ADC0, U1A pin 9 */
#define SB_KAE8_BATTERY_R_HIGH_OHM               (120000u)
#define SB_KAE8_BATTERY_R_LOW_OHM                (47000u)

/* Active-low user keys from sheet 3, routed on sheet 1.
 * Quectel GPIO Configuration V1.1 maps physical pins to GPIO[x] as below:
 *   pin 54 GPIO5 -> GPIO[57]
 *   pin 55 GPIO6 -> GPIO[87]
 *   pin 81 GPIO8 -> GPIO[8]
 */
#define SB_KAE8_KEY_VOLUME_UP_GPIO               GPIO_PIN_NO_57 /* SW1 -> U1B pin 54, GPIO5 -> GPIO[57] */
#define SB_KAE8_KEY_VOLUME_DOWN_GPIO             GPIO_PIN_NO_87 /* SW2 -> U1B pin 55, GPIO6 -> GPIO[87] */
#define SB_KAE8_KEY_MODE_GPIO                    GPIO_PIN_NO_8  /* SW3 -> U1C pin 81, GPIO8 -> GPIO[8] */
#define SB_KAE8_KEY_ACTIVE_LEVEL                 PIN_LEVEL_LOW
#define SB_KAE8_KEY_IDLE_PULL                    PIN_PULL_PU

/* User LED. USER_LED_1 -> U1C physical pin 83, GPIO9 -> GPIO[69].
 * Q5 is a low-side transistor, so GPIO high turns LD2 on.
 */
#define SB_KAE8_STATUS_LED_GPIO                  GPIO_PIN_NO_69
#define SB_KAE8_STATUS_LED_ON_LEVEL              PIN_LEVEL_HIGH
#define SB_KAE8_STATUS_LED_OFF_LEVEL             PIN_LEVEL_LOW

/* Speaker PA shutdown/control path. SPK_SHDN -> U1A physical pin 22, MAIN_CTS -> GPIO[54].
 * Signal name is SPK_SHDN; high asserts shutdown.
 */
#define SB_KAE8_SPK_SHDN_GPIO                    GPIO_PIN_NO_54
#define SB_KAE8_SPK_SHDN_ASSERT_LEVEL            PIN_LEVEL_HIGH
#define SB_KAE8_SPK_SHDN_DEASSERT_LEVEL          PIN_LEVEL_LOW

/* External NOR is not used by the clean MQTT demo build. */

#ifdef __cplusplus
}
#endif

#endif /* SB_BOARD_KAE8_SQ1_H */
